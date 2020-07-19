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

static int _save_data(const char *key, struct pair *params, int add)
{
	cJSON *cfg;
	cJSON *c_params;
	cJSON *obj;
	int i;

	if (add) {
		cfg = cJSON_Parse(poly->config);
		c_params = cJSON_DetachItemFromObject(cfg, "customParams");
	} else {
		c_params = cJSON_CreateObject();
	}

	while (params) {
		// Create a new object and add it to c_params?
		cJSON_AddStringToObject(c_params, params->key, params->value);
		params = params->next;
	}

	i = 0;
	while (key[i]) {
		tolower(key[i]);
		i++;
	}


	/* Send new c_params object to Polyglot */
	obj = cJSON_CreateObject();
	cJSON_AddItemToObject(obj, key, c_params);
	loggerf(DEBUG, "Sending %s\n", cJSON_Print(obj));
	poly_send(obj);
	cJSON_Delete(obj);

	if (add)
		cJSON_Delete(cfg);

	return 0;
}

static char *_get_data(const char *dtype, char *key)
{
	cJSON *cfg;
	cJSON *params;
	cJSON *item;
	char *value = NULL;
	int i;

	cfg = cJSON_Parse(poly->config);
	params = cJSON_GetObjectItem(cfg, dtype);
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

static int _remove_data(const char *dtype, char *key)
{
	cJSON *cfg;
	cJSON *item;
	cJSON *update;
	cJSON *params;
	cJSON *obj;
	int i;

	cfg = cJSON_Parse(poly->config);

	update = cJSON_CreateObject();
	params = cJSON_DetachItemFromObject(cfg, dtype);
	if (cJSON_IsObject(params)) {
		for (i = 0; i < cJSON_GetArraySize(params); i++) {
			item = cJSON_GetArrayItem(params, i);
			if (strcmp(item->string, key) != 0)
				cJSON_AddStringToObject(update, item->string, item->valuestring);
		}
	}

	i = 0;
	while (dtype[i]) {
		tolower(dtype[i]);
		i++;
	}

	/* Send updated object to Polyglot */
	obj = cJSON_CreateObject();
	cJSON_AddItemToObject(obj, dtype, update);
	loggerf(DEBUG, "Updating %s = %s\n", dtype, cJSON_Print(obj));
	poly_send(obj);
	cJSON_Delete(obj);
	cJSON_Delete(update);
	cJSON_Delete(cfg);

	return 0;
}

void freeCustomPairs(struct pair *params)
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
		for (i = 0; i < cJSON_GetArraySize(params); i++) {
			item = cJSON_GetArrayItem(params, i);
			tmp = malloc(sizeof(struct pair));
			tmp->key = strdup(item->string);
			tmp->value = strdup(item->valuestring);
			tmp->flags = 0;
			tmp->next = p;
			p = tmp;
		}
	} else {
		logger(ERROR, "No customParams available.\n");
	}

	cJSON_Delete(cfg);

	return p;
}

/*
 * getCustomParam
 *
 * Get the value of a key in the custom parameter set.
 */
char *getCustomParam(char *key)
{
	return _get_data("customParams", key);
}

/*
 * saveCustomParams
 *
 * Replace the existing custom parameter config with a new
 * list of key/value pairs.
 */
int saveCustomParams(struct pair *params)
{
	return _save_data("customParams", params, 0);
}

/*
 * addCustomParams
 *
 * Add additional custom parameters to the existing set of 
 * custom parameters.
 */
int addCustomParams(struct pair *params)
{
	return _save_data("customParams", params, 1);
}

/*
 * removeCustomParam
 *
 * Remove a single custom parameter based on the parameter
 * key.
 */
int removeCustomParam(char *key)
{
	return _remove_data("customParams", key);
}

/*
 * saveCustomData
 *
 * Replace the existing custom data config with a new
 * list of key/value pairs.
 */
int saveCustomData(struct pair *params)
{
	return _save_data("customData", params, 0);
}

/*
 * addCustomData
 *
 * Add additional custom data to the existing set of 
 * custom data.
 */
int addCustomData(struct pair *params)
{
	return _save_data("customData", params, 1);
}

/*
 * getCustomData
 *
 * Get the value of a key in the custom data set.
 */
char *getCustomData(char *key)
{
	return _get_data("customData", key);
}

/*
 * removeCustomData
 *
 * Remove a single custom data based on the data key.
 */
int removeCustomData(char *key)
{
	return _remove_data("customData", key);
}
