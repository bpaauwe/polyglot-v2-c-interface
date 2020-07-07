#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
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
 * isConnected
 *
 * Returns:
 *    true if we have a mqtt connection to polyglot
 *    false if we don't have an active mqtt connection to polyglot
 */
int isConnected(void)
{
	return poly->connected;
}

/*
 * getConfig
 *
 * Returns:
 *    A JSON string with the latest config from polyglot.
 *    The string is a copy of the internal version and must be
 *    free'd by the caller.
 */
char *getConfig(void)
{
	// Need to store the config somewhere. We've been storing it in profile,
	// but that's just something we're passing around the mqtt functions
	// should we maintain a global pointer to this also?
	return strdup(poly->config);
}

/*
 * getCustomParams and getCustomParam
 *
 * Return the either the specific custom parameter value or
 * the list of key/value pairs.
 *
 * How do we want to return the list?
 *   - JSON dictionary would be easy, but caller would then have to parse
 *     the JSON.
 *   - Could be an array of key/values. would need to also return the length
 *   - linked list of key/value pairs. Most effort here, but possibly easy
 *     for caller.  Would need a key/value struct type.
 */
struct pair *getCustomParams(void)
{
	cJSON *cfg;
	cJSON *params;
	cJSON *item;
	struct pair *p = NULL;
	struct pair *tmp;
	int i;

	// Pull this from existing config structure
	cfg = cJSON_Parse(poly->config);
	params = cJSON_GetObjectItem(cfg, "customParams");

	// params is a list of objects, not an array, how do we iterate this?
	if (cJSON_IsObject(params)) {
		loggerf(INFO, "Found Params %s\n", cJSON_Print(params));
		for (i = 0; i < cJSON_GetArraySize(params); i++) {
			item = cJSON_GetArrayItem(params, i);
			tmp = malloc(sizeof(struct pair));
			tmp->key = strdup(item->string);
			tmp->value = strdup(item->valuestring);
			tmp->flags = 0;
			tmp->next = p;
			p = tmp;
			loggerf(INFO, "Found customParam %s -> %s\n", item->string, item->valuestring);
		}
	} else {
		logger(ERROR, "No customParams.\n");
	}

	cJSON_Delete(cfg);

	return p;
}

char *getCustomParam(char *key)
{
	cJSON *cfg;
	cJSON *params;
	cJSON *item;
	char *value = NULL;
	int i;

	cfg = cJSON_Parse(poly->config);
	params = cJSON_GetObjectItem(cfg, "customParams");
	if (cJSON_IsObject(params)) {
		for (i = 0; i < cJSON_GetArraySize(params); i++) {
			item = cJSON_GetArrayItem(params, i);
			if (strcmp(item->string, key) == 0) {
				value = strdup(item->valuestring);
			}
		}
	}

	cJSON_Delete(cfg);

	return value;
}

void freeCustomParams(struct pair *params)
{
	struct pair *p;
	
	while (params) {
		free(params->key);
		free(params->value);
		p = params->next;
		free(params);
		params = p;
	}
}

/*
 * saveCustomParams
 *
 * Replace the existing custom parameter config with a new
 * list of key/value pairs.
 */
int saveCustomParams(struct pair *params)
{
	return 0;
}

/*
 * addCustomParams
 *
 * Add additional custom parameters to the existing set of 
 * custom parameters.
 */
int addCustomParams(struct pair *params)
{
	cJSON *cfg;
	cJSON *item;
	cJSON *c_params;
	cJSON *obj;

	cfg = cJSON_Parse(poly->config);
	c_params = cJSON_DetachItemFromObject(cfg, "customParams");
	while (params) {
		// Create a new object and add it to c_params?
		cJSON_AddStringToObject(c_params, params->key, params->value);
		params = params->next;
	}

	loggerf(INFO, "New custom params = %s\n", cJSON_Print(c_params));

	/* Send new c_params object to Polyglot */
	obj = cJSON_CreateObject();
	cJSON_AddItemToObject(obj, "customparams", c_params);
	poly_send(obj);
	cJSON_Delete(obj);

	cJSON_Delete(cfg);

	return 0;
}

/*
 * removeCustomParam
 *
 * Remove a single custom parameter based on the parameter
 * key.
 */
int removeCustomParam(char *key)
{
	cJSON *cfg;
	cJSON *item;
	cJSON *update;
	cJSON *params;
	cJSON *obj;
	int i;

	cfg = cJSON_Parse(poly->config);

	update = cJSON_CreateObject();
	params = cJSON_DetachItemFromObject(cfg, "customParams");
	if (cJSON_IsObject(params)) {
		for (i = 0; i < cJSON_GetArraySize(params); i++) {
			item = cJSON_GetArrayItem(params, i);
			if (strcmp(item->string, key) != 0)
				cJSON_AddStringToObject(update, item->string, item->valuestring);
		}
	}

	/* Send updated params object to Polyglot */
	obj = cJSON_CreateObject();
	cJSON_AddItemToObject(obj, "customparams", update);
	loggerf(INFO, "Updating custom paramters = %s\n", cJSON_Print(obj));
	poly_send(obj);
	cJSON_Delete(obj);
	cJSON_Delete(update);
	cJSON_Delete(cfg);

	return 0;
}
