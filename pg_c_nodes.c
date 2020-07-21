/*
  Copyright (c) 2020 Robert Paauwe

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <netdb.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <mosquitto.h>
#include "cJSON.h"
#include "c_interface.h"
#include "c_int_interface.h"

extern struct mosquitto *mosq;
extern struct profile *poly;

static void free_node(struct node *n)
{
	if (n->drivers)
		free(n->drivers);
	if (n->commands)
		free(n->commands);
	if (n->sends)
		free(n->sends);

	loggerf(DEBUG, "Freeing node %s\n", n->name);
	free(n);

	return;
}

static void node_set_driver(struct node *n, char *driver, char *value, int report, int force, int uom)
{
	struct driver *d;
	int cnt;
	int changed = 0;

	d = n->drivers;
	for (cnt = 0; cnt < n->driver_cnt; cnt++) {
		if (strcmp(d->driver, driver) == 0) {
			if (strcmp(d->value, value) != 0 || d->uom != uom)
				changed = 1;
			d->value = value;
			d->uom = uom;
			if (report)
				n->ops.reportDriver(n, d->driver, changed, force);
			return;
		}
		d++;
	}
	return;
}

static void node_report_driver(struct node *n, char *drv, int changed, int force)
{
	struct driver *d;
	int cnt;
	cJSON *obj;
	cJSON *status;

	d = n->drivers;
	for (cnt = 0; cnt < n->driver_cnt; cnt++) {
		if (strcmp(d->driver, drv) == 0) {
			if (changed || force) {
				/* Create message and send */
				status = cJSON_CreateObject();
				cJSON_AddStringToObject(status, "address", n->address);
				cJSON_AddStringToObject(status, "driver", d->driver);
				cJSON_AddStringToObject(status, "value", d->value);
				cJSON_AddNumberToObject(status, "uom", d->uom);

				obj = cJSON_CreateObject();
				cJSON_AddItemToObject(obj, "status", status);

				poly_send(obj);

				cJSON_Delete(obj);
			}
			return;
		}
		d++;
	}
	return;
}

static void node_report_drivers(struct node *n)
{
	int cnt;
	cJSON *obj;
	cJSON *status;

	for (cnt = 0; cnt < n->driver_cnt; cnt++) {
		/* Create message and send */
		status = cJSON_CreateObject();
		cJSON_AddStringToObject(status, "address", n->address);
		cJSON_AddStringToObject(status, "driver", n->drivers[cnt].driver);
		cJSON_AddStringToObject(status, "value", n->drivers[cnt].value);
		cJSON_AddNumberToObject(status, "uom", n->drivers[cnt].uom);

		obj = cJSON_CreateObject();
		cJSON_AddItemToObject(obj, "status", status);

		poly_send(obj);

		cJSON_Delete(obj);
	}
	return;
}

static void node_report_cmd(struct node *n, char *sends, char *value, int uom)
{
	struct send *s;
	int cnt;
	cJSON *obj;
	cJSON *cmd;

	s = n->sends;
	/* look up command in sends array */
	for (cnt = 0; cnt < n->send_cnt; cnt++) {
		if (strcmp(s->id, sends) == 0) {
			/* Create message and send */
			cmd = cJSON_CreateObject();
			cJSON_AddStringToObject(cmd, "address", n->address);
			cJSON_AddStringToObject(cmd, "command", s->id);
			cJSON_AddNumberToObject(cmd, "uom", uom);
			if (value) 
				cJSON_AddStringToObject(cmd, "value", value);

			obj = cJSON_CreateObject();
			cJSON_AddItemToObject(obj, "command", cmd);
			poly_send(obj);
			cJSON_Delete(obj);
			return;
		}
		s++;
	}
	return;
}

static void node_query(struct node *n)
{
	node_report_drivers(n);
}

static const struct node_ops node_functions = {
	.setDriver = node_set_driver,
	.reportDriver = node_report_driver,
	.reportDrivers = node_report_drivers,
	.reportCmd = node_report_cmd,
	.query = node_query,
	.start = NULL,
	.shortPoll = NULL,
	.longPoll = NULL,
	.status = NULL,
};


/*
 * allocNode
 *
 * A helper function to allocate and do basic configuration of a node
 * structure.
 *
 * returns an allocated node.
 */
struct node *allocNode(char *id, char *primary, char *address, char *name)
{
	struct node *new_node;

	new_node = (struct node *)malloc(sizeof(struct node));
	if (!new_node)
		return NULL;

	memset(new_node, 0, sizeof(struct node));

	new_node->id = id;
	new_node->name = name;

	new_node->primary = malloc(14);
	new_node->address = malloc(14);
	sprintf(new_node->primary, "%s", primary);
	sprintf(new_node->address, "%s", address);

	if (strcmp(primary, address) == 0)
		new_node->isPrimary = 1;
	else
		new_node->isPrimary = 0;

	new_node->ops = node_functions;

	new_node->added = 0;
	new_node->enabled = 0;
	new_node->driver_cnt = 0;
	new_node->command_cnt = 0;
	new_node->send_cnt = 0;
	new_node->drivers = NULL;
	new_node->next = NULL;

	return new_node;
}

/*
 * addDriver
 *
 * Add a driver to the node's driver list
 */
void addDriver(struct node *n, char *driver, char *init, int uom)
{
	struct driver *d;
	struct driver *nd;
	int cnt = 0;

	/* 1. Count how many drivers are in current node driver array */
	cnt = n->driver_cnt;
	loggerf(DEBUG, "node %s has %d drivers\n", n->name, n->driver_cnt);

	/* 2. Allocate a new array sized to include new driver */
	nd = calloc(cnt + 1, sizeof(struct driver));

	/* 3. Copy existing drivers to new array */
	memcpy(nd, n->drivers, (cnt * sizeof(struct driver)));

	/* 4. Add new driver in last slot */
	nd[cnt].driver = driver;
	nd[cnt].value = init;
	nd[cnt].uom = uom;

	/* 5. Replace node's driver array with new one */
	free(n->drivers);
	n->drivers = nd;
	n->driver_cnt++;

	d = n->drivers;
	for (cnt = 0; cnt < n->driver_cnt; cnt++) {
		loggerf(DEBUG, "%s, %s, %d\n", d->driver, d->value, d->uom);
		d++;
	}

	return;
}

/*
 * addCommand
 *
 * add a command to the node's command list.
 */
void addCommand(struct node *n, char *cmd_id,
		void (*callback)(struct node *n, char *cmd, char *value, int uom))
{
	struct command *nc;
	int cnt = 0;

	cnt = n->command_cnt;
	loggerf(DEBUG, "node %s has %d commands\n", n->name, n->command_cnt);

	nc = calloc(cnt + 1, sizeof(struct command));
	memcpy(nc, n->commands, (cnt * sizeof(struct command)));

	nc[cnt].id = cmd_id;
	nc[cnt].callback = callback;
	
	free(n->commands);
	n->commands = nc;
	n->command_cnt++;

	return;
}

/*
 * addSend
 *
 * add a command that gets sent to the node's sent list.
 */
void addSend(struct node *n, char *cmd_id,
		void (*callback)(struct node *n, char *cmd, char *value, int uom))
{
	struct send *nc;
	int cnt = 0;

	cnt = n->send_cnt;
	loggerf(DEBUG, "node %s has %d sends\n", n->name, n->send_cnt);

	nc = calloc(cnt + 1, sizeof(struct send));
	memcpy(nc, n->sends, (cnt * sizeof(struct send)));

	nc[cnt].id = cmd_id;
	nc[cnt].callback = callback;
	
	free(n->sends);
	n->sends = nc;
	n->send_cnt++;

	return;
}


/*
 * addNode
 *
 * Add a node to the node server's node list
 */
void addNode(struct node *n)
{
	struct node *tmp;
	cJSON *node;
	cJSON *obj;
	cJSON *drv;
	cJSON *drv_array;
	cJSON *node_array;
	cJSON *node_array_obj;
	int cnt;

	if (!poly->nodelist) {
		poly->nodelist = n;
	} else {

		tmp = poly->nodelist;
		while (tmp->next)
			tmp = tmp->next;

		tmp->next = n;
	}

	/* Send node info to Polyglot */

	node = cJSON_CreateObject();
	cJSON_AddStringToObject(node, "address", n->address);
	cJSON_AddStringToObject(node, "name", n->name);
	cJSON_AddStringToObject(node, "node_def_id", n->id);
	cJSON_AddStringToObject(node, "primary", n->primary);
	cJSON_AddNumberToObject(node, "hint", n->hint);
	drv_array = cJSON_AddArrayToObject(node, "drivers");
	for (cnt = 0; cnt < n->driver_cnt; cnt++) {
		drv = cJSON_CreateObject();
		cJSON_AddStringToObject(drv, "driver", n->drivers[cnt].driver);
		cJSON_AddStringToObject(drv, "value", n->drivers[cnt].value);
		cJSON_AddNumberToObject(drv, "uom", n->drivers[cnt].uom);
		cJSON_AddItemToArray(drv_array, drv);
	}

	node_array_obj = cJSON_CreateObject();
	node_array = cJSON_AddArrayToObject(node_array_obj, "nodes");
	cJSON_AddItemToArray(node_array, node);

	obj = cJSON_CreateObject();
	cJSON_AddItemToObject(obj, "addnode", node_array_obj);

	poly_send(obj);
	cJSON_Delete(obj);

	return;
}

/*
 * delNode
 *
 * Delete a node from the internal list and ask polyglot to
 * also delete the node.
 */
void delNode(char *address)
{
	struct node *tmp;
	struct node *prev;
	cJSON *obj, *addr;

	/* Ask polyglot to delete the node */
	obj = cJSON_CreateObject();
	addr = cJSON_CreateObject();
	cJSON_AddStringToObject(addr, "address", address);
	cJSON_AddItemToObject(obj, "removenode", addr);
	loggerf(DEBUG, "Calling polyglot to delete node %s\n", address);
	poly_send(obj);
	cJSON_Delete(obj);


	/* Delete node from internal node list */
	if (poly->nodelist) {
		tmp = poly->nodelist;
		if (strcmp(tmp->address, address) == 0) {
			poly->nodelist = tmp->next;
			free_node(tmp);
		} else {
			prev = tmp;
			tmp = tmp->next;
			while (tmp) {
				if (strcmp(tmp->address, address) == 0) {
					prev->next = tmp->next;
					free_node(tmp);
				}
				tmp = tmp->next;
			}
		}
	}
	return;
}

/*
 * getNode
 *
 * Retrieve a specific node by address.
 *
 * Python version returns the node 'dictionary' from the config. I think
 * this should return a pointer to the node structure.
 *
 * Returns pointer to node structure.
 */
struct node *getNode(char *address)
{
	struct node *tmp;

	if (poly->nodelist) {
		tmp = poly->nodelist;
		while (tmp) {
			if (strcmp(tmp->address, address) == 0)
				return tmp;
			tmp = tmp->next;
		}
		loggerf(ERROR, "Node address %s does not exist in node list\n", address);
	} else {
		logger(ERROR, "Node list does not exist.\n");
	}

	return NULL;
}

/*
 * getNodes
 *
 * Return a pointer to the node list
 */
struct node *getNodes(void)
{
	return poly->nodelist;
}

/*
 * node_cmd_exec
 *
 * An internal function that executes a node command by
 * looking up the command in the node's command list and
 * calling the command callback.
 */
void *node_cmd_exec(void *args)
{
	cJSON *msg = (cJSON *)args;
	cJSON *addr, *cmd, *value, *uom;
	struct node *tmp;
	int i;

	addr = cJSON_GetObjectItem(msg, "address");
	cmd = cJSON_GetObjectItem(msg, "cmd");
	if (!cJSON_HasObjectItem(msg, "value"))
		cJSON_AddStringToObject(msg, "value", "");
	value = cJSON_GetObjectItem(msg, "value");

	if (!cJSON_HasObjectItem(msg, "uom"))
		cJSON_AddStringToObject(msg, "uom", "0");
	uom = cJSON_GetObjectItem(msg, "uom");

	loggerf(DEBUG, "Process command %s\n", cJSON_Print(msg));

	if (poly->nodelist) {
		tmp = poly->nodelist;
		while (tmp) {
			/* look up the node with this address */
			if (strcmp(tmp->address, addr->valuestring) == 0) {
				/*
				 * call command callback with cmd->valuestring,
				 * value->valuestring, and uom->valuestring
				 */
				for (i = 0; i < tmp->command_cnt; i++) {
					if (strcmp(tmp->commands[i].id, cmd->valuestring) == 0) {
						int iuom = 0;

						if (uom->valuestring)
							iuom = atoi(uom->valuestring);

						loggerf(DEBUG, "callback(%s, %s, %d)\n",
								cmd->valuestring,
								value->valuestring,
								iuom);
						tmp->commands[i].callback(tmp, cmd->valuestring,
								value->valuestring, iuom);
					}
				}
				return NULL;
			}
			tmp = tmp->next;
		}
	}

	return NULL;
}

void *node_query_exec(void *args)
{
	char *addr = (char *)args;
	struct node *tmp;

	if (poly->nodelist) {
		tmp = poly->nodelist;
		while (tmp) {
			if (strcmp(addr, "all") == 0 ||
					strcmp(addr, tmp->address) == 0) {
				if (tmp->ops.reportDrivers != NULL)
					tmp->ops.reportDrivers(tmp);
			}
		}
	}

	return NULL;
}

void *node_status_exec(void *args)
{
	char *addr = (char *)args;
	struct node *tmp;

	if (poly->nodelist) {
		tmp = poly->nodelist;
		while (tmp) {
			if (strcmp(addr, "all") == 0 ||
					strcmp(addr, tmp->address) == 0) {
				if (tmp->ops.reportDrivers != NULL)
					tmp->ops.reportDrivers(tmp);
			}
		}
	}

	return NULL;
}
