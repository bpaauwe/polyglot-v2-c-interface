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
#include <mkdio.h>
#include "cJSON.h"
#include "c_interface.h"
#include "c_int_interface.h"

extern struct mosquitto *mosq;
extern struct profile *poly;

#define CUSTOM_CONFIG_DOCS_FILE_NAME "POLYGLOT_CONFIG.md"
#define SERVER_JSON_FILE_NAME        "server.json"

/*
 * setCustomParamsDoc
 *
 * Get the custom parameter configuration documentation file
 * and send it to Polyglot.
 *
 * this is called when the config file is sent to the node server.
 */
void setCustomParamsDoc(void)
{
	FILE *fp;
	char *buffer = NULL;
	size_t len;
	MMIOT *mkdown;
	cJSON *msg;

	if (poly->custom_config_doc_sent)
		return;

	poly->custom_config_doc_sent = 1;

	// Get MD file
	fp = fopen(CUSTOM_CONFIG_DOCS_FILE_NAME, "r");
	if (fp == NULL) {
		loggerf(ERROR, "Failed to open config doc file %s\n",
				CUSTOM_CONFIG_DOCS_FILE_NAME);
		return;
	}

	// Convert markdown to HTML
	mkdown = gfm_in(fp, 0);
	mkd_compile(mkdown, 0);
	mkd_document(mkdown, &buffer);
	
	fclose(fp);

	// poly_send('{"customparamsdoc": doc}')
	msg = cJSON_CreateObject();
	cJSON_AddStringToObject(msg, "customparamsdoc", buffer);
	poly_send(msg);
	cJSON_Delete(msg);
	free(buffer);

	return;
}

/*
 * installProfile
 *
 * Install the profile files on the ISY
 */
void installProfile(void)
{
	cJSON *msg, *body;

	logger(INFO, "Sending Install Profile command to Polyglot.\n");
	body = cJSON_CreateObject();
	cJSON_AddBoolToObject(body, "reboot", 0); 
	msg = cJSON_CreateObject();
	cJSON_AddItemToObject(msg, "installprofile", body);
	poly_send(msg);
	cJSON_Delete(msg);

	return;
}

/*
 * restart
 *
 * Ask Polygot to restart this node server.
 */
void restart(void)
{
	cJSON *msg, *body;

	logger(INFO, "Asking Polyglot to restart me..\n");
	body = cJSON_CreateObject();
	msg = cJSON_CreateObject();
	cJSON_AddItemToObject(msg, "restart", body);
	poly_send(msg);
	cJSON_Delete(msg);

	return;
}
