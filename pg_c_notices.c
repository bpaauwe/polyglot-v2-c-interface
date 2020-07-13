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

/*
 * addNotice
 *
 * Tell Polyglot to display a notice.
 */
void addNotice(char *key, char *notice)
{
	cJSON *msg;
	cJSON *data;
	// message = {'addnotice': {'key': key, 'value': notice}}
	
	data = cJSON_CreateObject();
	cJSON_AddStringToObject(data, "key", key);
	cJSON_AddStringToObject(data, "value", notice);
	msg = cJSON_CreateObject();
	cJSON_AddItemToObject(msg, "addnotice", data);
	loggerf(INFO, "Adding notice: %s\n", cJSON_Print(msg));
	poly_send(msg);

	cJSON_Delete(msg);

	return;
}

/*
 * removeNotice
 *
 * Remove a single notice that was previously set.
 */
void removeNotice(char *key)
{
	cJSON *msg;
	cJSON *data;
	// message = {'removenotice': {'key': key}}

	data = cJSON_CreateObject();
	cJSON_AddStringToObject(data, "key", key);
	msg = cJSON_CreateObject();
	cJSON_AddItemToObject(msg, "removenotice", data);
	poly_send(msg);
	cJSON_Delete(msg);

	return;

}

/*
 * removeNoticesAll
 *
 * Remove all notices from Polyglot
 */
void removeNoticesAll(void)
{
	cJSON *cfg;
	cJSON *notices;
	cJSON *item;
	int i;
	
	cfg = cJSON_Parse(poly->config);
	notices = cJSON_GetObjectItem(cfg, "notices");

	if (notices) {
		loggerf(INFO, "found existing notices: %s\n", cJSON_Print(notices));
		// list of string objects
		for (i = 0; i < cJSON_GetArraySize(notices); i++) {
			item = cJSON_GetArrayItem(notices, i);
			loggerf(INFO, "removeNotice(%s)\n", item->string);
			removeNotice(item->string);
		}
	}

	return;
}

/*
 * getNotices
 *
 * Python API returns a JSON data structure from the config.
 * But, this should probably return a list of pairs since that
 * would be easier to use in a C program.
 */
struct pair *getNotices(void)
{
	cJSON *cfg;
	cJSON *notices;
	cJSON *item;
	struct pair *p = NULL, *tmp;
	int i;

	cfg = cJSON_Parse(poly->config);
	notices = cJSON_GetObjectItem(cfg, "notices");

	if (notices) {
		for (i = 0; i < cJSON_GetArraySize(notices); i++) {
			item = cJSON_GetArrayItem(notices, i);
			tmp = malloc(sizeof(struct pair));
			tmp->key = strdup(item->string);
			tmp->value = strdup(item->valuestring);
			tmp->flags = 0;
			tmp->next = p;
			p = tmp;
		}
	}

	return p;
}
