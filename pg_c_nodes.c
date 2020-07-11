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

void node_set_driver(struct node *n, char *driver, char *value, int report, int force, int uom)
{
	struct driver *d;
	int cnt;
	int changed = 0;

	logger(INFO, "TODO: node_set_driver implementation\n");
	d = n->drivers;
	for (cnt = 0; cnt < n->driver_cnt; cnt++) {
		if (strcmp(d->driver, driver) == 0) {
			if (strcmp(d->value, value) != 0 || d->uom != uom)
				changed = 1;
			d->value = value;
			d->uom = uom;
			if (report)
				n->ops.reportDriver(n, d, changed, force);
			return;
		}
		d++;
	}
	return;
}

void node_report_driver(struct node *n, struct driver *drv, int changed, int force)
{
	struct driver *d;
	int cnt;
	cJSON *obj;
	cJSON *status;

	logger(INFO, "TODO: node_report_driver implementation\n");
	d = n->drivers;
	for (cnt = 0; cnt < n->driver_cnt; cnt++) {
		if (strcmp(d->driver, drv->driver) == 0) {
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

static const struct node_ops node_functions = {
	.setDriver = node_set_driver,
	.reportDriver = node_report_driver,
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
	loggerf(INFO, "node %s has %d drivers\n", n->name, n->driver_cnt);

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
		loggerf(INFO, "%s, %s, %d\n", d->driver, d->value, d->uom);
		d++;
	}

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
