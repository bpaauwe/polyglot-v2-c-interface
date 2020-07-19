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

struct mosquitto *mosq = NULL;
struct profile *poly = NULL;

static void on_connect(struct mosquitto *m, void *ptr, int res);
static void on_message(struct mosquitto *m, void *ptr,
		const struct mosquitto_message *msg);
static void on_disconnect(struct mosquitto *m, void *ptr, int res);
static void on_publish(struct mosquitto *m, void *ptr, int mid);
static void on_subscribe(struct mosquitto *m, void *ptr, int mid, int qos, const int *granted);
static int get_stdin_info(char **host, int *port, int *profile);
static int get_stdin_info_test(char **host, int *port, int *profile);
extern void initialize_logging(void);


/*
 * Initialize the link to Polyglot
 */
int init(void (*start), void (*shortPoll), void (*longPoll), void (*onConfig))
{
	int ret;
	char *host;
	int port;
	int profile;

	//if (get_stdin_info(&host, &port, &profile) != 0)
	if (get_stdin_info_test(&host, &port, &profile) != 0)
		return -2;

	initialize_logging();

	poly = malloc(sizeof(struct profile));
	if (poly == NULL) {
		fprintf(stderr, "init: memory alloction failed for struct profile\n");
		return -3;
	}

	memset(poly, 0, sizeof(struct profile));
	poly->num = profile; 
	poly->config = NULL;
	poly->connected = 0;
	poly->custom_config_doc_sent = 0;
	poly->mqtt_info.profile_num = profile;
	poly->mqtt_info.start = start;
	poly->mqtt_info.longPoll = longPoll;
	poly->mqtt_info.shortPoll = shortPoll;
	poly->mqtt_info.onConfig = onConfig;
	poly->nodelist = NULL;

	/* Create runtime instance with random client ID */
	/*  client name, true, priv_data */
	mosq = mosquitto_new(NULL, true, (void *)&poly->mqtt_info);
	if (!mosq) {
		fprintf(stderr, "Failed to initialize a MQTT instance.\n");
		return -1;
	}

	/* Set callbacks */
	logger(DEBUG, "Configure MQTT callbacks\n");
	mosquitto_connect_callback_set(mosq, on_connect);
	mosquitto_message_callback_set(mosq, on_message);
	mosquitto_disconnect_callback_set(mosq, on_disconnect);
	mosquitto_subscribe_callback_set(mosq, on_subscribe);
	mosquitto_publish_callback_set(mosq, on_publish);

	/*
	 * This will be a secure connection.  The certificate files
	 * will either come from stdin or from home directory of the
	 * user running this.
	 */
	//mosquitto_tls_insecure_set(mosq, 1);
	ret = mosquitto_tls_set(mosq,
			  "/usr/home/bpaauwe/ssl/polyglot.crt",
			  NULL,
			  "/usr/home/bpaauwe/ssl/client.crt",
			  "/usr/home/bpaauwe/ssl/client_private.key",
			  NULL);
	if (ret == 3) {
		ret = mosquitto_tls_set(mosq,
				"/var/polyglot/ssl/polyglot.crt",
				NULL,
				"/var/polyglot/ssl/client.crt",
				"/var/polyglot/ssl/client_private.key",
				NULL);
	}
	mosquitto_tls_opts_set(mosq, 0, NULL, NULL);

	/* Make the connection */
	loggerf(INFO, "Connect to Polyglot via MQTT %s:%d\n", host, port);
	ret = mosquitto_connect_async(mosq, host, port, 0);
	switch (ret) {
		case MOSQ_ERR_SUCCESS:
			logger(INFO, "successful connection.\n");
			break;
		case MOSQ_ERR_ERRNO:
			loggerf(ERROR, "can't connect: %s\n", strerror(errno));
			return -1;
		case MOSQ_ERR_INVAL:
			logger(ERROR, "can't connect: invalid arguments\n");
			return -1;
		default:
			loggerf(ERROR, "can't connect: %s (%d)\n", mosquitto_strerror(ret), ret);
			return -1;
	}

	/* Start a thread to monitor the connection */
	logger(INFO, "Start mosquitto loop\n");
	ret = mosquitto_loop_start(mosq);
	//ret = mosquitto_loop_forever(mosq, 1000, 1000);

	return 0;
}

#define POLYGLOT_CONNECTION  "udi/polyglot/connections/polyglot"
#define POLYGLOT_INPUT "udi/polyglot/ns/%d"
#define POLYGLOT_SELFCONNECTION "udi/polyglot/connections/%d"

void poly_send(cJSON *msg)
{
	cJSON *node;
	char *msg_str;
	char topic[30];
	int ret;

	if (!cJSON_HasObjectItem(msg, "node")) {
		node = cJSON_CreateNumber(poly->num);
		cJSON_AddItemToObject(msg, "node", node);
	}

	sprintf(topic, POLYGLOT_SELFCONNECTION, poly->num);
	msg_str = cJSON_PrintUnformatted(msg);
	loggerf(DEBUG, "Publishing '%s' to %s\n", msg_str, topic);
	ret = mosquitto_publish(mosq, NULL, topic, strlen(msg_str), msg_str, 0, 0);
	if (ret)
		logger(ERROR, "Failed to publish message to Polyglot\n");
}

/*
 * Once the connection is established, subscribe to the necessary topics
 */

static void on_connect(struct mosquitto *m, void *ptr, int res)
{
	char msg[30];
	char topic[30];
	int ret;
	struct mqtt_priv *p = (struct mqtt_priv *)ptr;
	(void)res;

	ret = mosquitto_subscribe(m, NULL, POLYGLOT_CONNECTION, 0);

	sprintf(topic, POLYGLOT_INPUT, p->profile_num);
	mosquitto_subscribe(m, NULL, topic, 0);

	/* publish a message to kick things off */
	sprintf(topic, POLYGLOT_SELFCONNECTION, p->profile_num);
	sprintf(msg, "{\"node\": %d, \"connected\": true}", p->profile_num);
	ret = mosquitto_publish(m, NULL, topic, strlen(msg), msg, 0, 0);

	poly->connected = 1;
}

static void on_disconnect(struct mosquitto *m, void *ptr, int res)
{
	(void)m;
	(void)ptr;
	(void)res;

	logger(INFO, "on_disconnect() called. MQTT connection has dropped\n");
	poly->connected = 0;
}

static void on_message(struct mosquitto *m, void *ptr,
		const struct mosquitto_message *msg)
{
	struct mqtt_priv *p = (struct mqtt_priv *)ptr;
	pthread_t thread;
	cJSON *jmsg;
	cJSON *key;
	(void)m;

	if (msg == NULL) {
		printf("-- got NULL message\n");
		return;
	}

	loggerf(DEBUG, "-- got message @ %s: (%d, Qos %d, %s) '%s'\n",
			msg->topic, msg->payloadlen, msg->qos, msg->retain ? "R" : "!r",
			msg->payload);

	jmsg = cJSON_Parse(msg->payload);
	key = cJSON_GetObjectItemCaseSensitive(jmsg, "node");
	if (strcmp(key->valuestring, "polyglot") != 0) {
		/* ignore messsages not from polyglot */
		return;
	}

	/*
	 * Parse message and invoke handlers
	 *
	 * most of the handlers are executed in a thread so we don't block
	 * here.
	 */
	if (cJSON_HasObjectItem(jmsg, "connected")) {
		/* call start callback */
		if (p->start) {
			int ret;
			ret = pthread_create(&thread, NULL, p->start, NULL);
		}
	} else if (cJSON_HasObjectItem(jmsg, "config")) {
		/* store config object and call onConfig */
		key = cJSON_GetObjectItem(jmsg, "config");

		/* Call setCustomParamsDoc here */
		setCustomParamsDoc();

		poly->config = cJSON_Print(key);
		if (p->onConfig) {
			pthread_create(&thread, NULL, p->onConfig, (void *)poly->config);
		}
	} else if (cJSON_HasObjectItem(jmsg, "shortPoll")) {
		/* Call the node server's shortPoll callback */
		if (p->shortPoll)
			pthread_create(&thread, NULL, p->shortPoll, NULL);
	} else if (cJSON_HasObjectItem(jmsg, "longPoll")) {
		/* Call the node server's longPoll callback */
		if (p->longPoll)
			pthread_create(&thread, NULL, p->longPoll, NULL);
	} else if (cJSON_HasObjectItem(jmsg, "command")) {
		/* Execute the node command */
		cJSON *cmd = cJSON_GetObjectItem(jmsg, "command");
		pthread_create(&thread, NULL, node_cmd_exec, (void *)cmd);
	} else {
		// result, query, status, delete
		logger(DEBUG, "Message type not yet handled\n");
	}
}

static void on_subscribe(struct mosquitto *m, void *ptr, int mid, int qos, const int *granted)
{
	(void)m;
	(void)ptr;
	(void)mid;
	(void)qos;
	(void)granted;

	return;
}

static void on_publish(struct mosquitto *m, void *ptr, int res)
{
	(void)m;
	(void)ptr;
	(void)res;

	return;
}

static int get_stdin_info(char **host, int *port, int *profile)
{
	char *line = NULL;
	size_t linecap = 0;
	ssize_t linelen;
	cJSON *msg;
	const cJSON *key;

	linelen = getline(&line, &linecap, stdin);
	if (linelen > 0) {
		msg = cJSON_Parse(line);
		key = cJSON_GetObjectItemCaseSensitive(msg, "mqttHost");
		*host = malloc(strlen(key->valuestring) + 1);
		strcpy(*host, key->valuestring);

		key = cJSON_GetObjectItemCaseSensitive(msg, "mqttPort");
		*port = atoi(key->valuestring);

		key = cJSON_GetObjectItemCaseSensitive(msg, "profileNum");
		*profile = atoi(key->valuestring);

		cJSON_Delete(msg);
		free(line);
		return 0;
	}
	free(line);
	return -1;
}

/*
 * This makes testing a little easier. Using this we hard code
 * the values that would normally be written by polyglot when it
 * starts the node server.  By hard coding the values, we can 
 * start the node server from the command line and not have to start
 * it from the Polyglot dashboard.  This also means that stderr
 * output will go the console instead of the Polyglot log.
 */
static int get_stdin_info_test(char **host, int *port, int *profile)
{
	//*host = strdup("192.158.92.11");
	*host = strdup("localhost");
	*port = 1883;
	*profile = 17;
	return 0;
}
