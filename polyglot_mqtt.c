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

struct mosquitto *mosq = NULL;
int polyglot_connected = 0;

static void on_connect(struct mosquitto *m, void *ptr, int res);
static void on_message(struct mosquitto *m, void *ptr,
		const struct mosquitto_message *msg);
static void on_disconnect(struct mosquitto *m, void *ptr, int res);
static void on_publish(struct mosquitto *m, void *ptr, int mid);
static void on_subscribe(struct mosquitto *m, void *ptr, int mid, int qos, const int *granted);
static int get_stdin_info(char **host, int *port, int *profile);
static int get_stdin_info_test(char **host, int *port, int *profile);
extern void initialize_logging(void);

struct profile {
	int num;
	char *config;
	int junk;
	void *(*start)(void *args);
	void *(*longPoll)(void *args);
	void *(*shortPoll)(void *args);
	void *(*onConfig)(void *args);
};

/*
 * Initialize the link to Polyglot
 */
int init(void (*start), void (*shortPoll), void (*longPoll), void (*onConfig))
{
	char msg[50];
	int ret;
	struct profile *p;
	char *host;
	int port;
	int profile;

	//if (get_stdin_info(&host, &port, &profile) != 0)
	if (get_stdin_info_test(&host, &port, &profile) != 0)
		return -2;

	initialize_logging();

	p = malloc(sizeof(struct profile));
	if (p == NULL) {
		fprintf(stderr, "init: memory alloction failed for struct profile\n");
		return -3;
	}

	memset(p, 0, sizeof(struct profile));
	p->num = profile;  // TODO: get this from stdin?
	p->config = NULL;
	p->junk = 1234;
	p->start = start;
	p->longPoll = longPoll;
	p->shortPoll = shortPoll;
	p->onConfig = onConfig;

	/* Create runtime instance with random client ID */
	/*  client name, true, priv_data */
	mosq = mosquitto_new(NULL, true, (void *)p);
	if (!mosq) {
		fprintf(stderr, "Failed to initialize a MQTT instance.\n");
		return -1;
	}

	/* Set callbacks */
	logger(INFO, "Configure MQTT callbacks\n");
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
	loggerf(INFO, "tls set returned %d\n", ret);
	mosquitto_tls_opts_set(mosq, 0, NULL, NULL);

	/* Make the connection */
	/* TODO: Host and port should come from stdin */
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

	logger(INFO, "finished with interface init\n");
	return 0;
}

/*
 * Once the connection is established, subscribe to the necessary topics
 *
 * TODO: Need to have the profile number pulled from ptr.
 */
#define POLYGLOT_CONNECTION  "udi/polyglot/connections/polyglot"
#define POLYGLOT_INPUT "udi/polyglot/ns/%d"
#define POLYGLOT_SELFCONNECTION "udi/polyglot/connections/%d"

static void on_connect(struct mosquitto *m, void *ptr, int res)
{
	char msg[30];
	char topic[30];
	int ret;
	struct profile *p = (struct profile *)ptr;

	loggerf(INFO, "Subscribing to node server profile %d\n", p->num);

	ret = mosquitto_subscribe(m, NULL, POLYGLOT_CONNECTION, 0);
	loggerf(INFO, "subscribe returned %d\n", ret);

	sprintf(topic, POLYGLOT_INPUT, p->num);
	mosquitto_subscribe(m, NULL, topic, 0);

	//sprintf(topic, NODESERVER_TOPIC, p->num);
	//mosquitto_subscribe(m, NULL, topic, 0);

	/* publish a message to kick things off */
	// { node: this.profile, connected: true }
	sprintf(topic, POLYGLOT_SELFCONNECTION, p->num);
	sprintf(msg, "{\"node\": %d, \"connected\": true}", p->num);
	ret = mosquitto_publish(m, NULL, topic, strlen(msg), msg, 0, 0);
	loggerf(INFO, "Published: %s\n", msg);

	polyglot_connected = 1;
}

static void on_disconnect(struct mosquitto *m, void *ptr, int res)
{
	logger(INFO, "on_disconnect() called. MQTT connection has dropped\n");
	polyglot_connected = 0;
}

static void on_message(struct mosquitto *m, void *ptr,
		const struct mosquitto_message *msg)
{
	struct profile *p = (struct profile *)ptr;
	pthread_t thread;
	cJSON *jmsg;
	cJSON *key;

	if (msg == NULL) {
		printf("-- got NULL message\n");
		return;
	}

	loggerf(INFO, "-- got message @ %s: (%d, Qos %d, %s) '%s'\n",
			msg->topic, msg->payloadlen, msg->qos, msg->retain ? "R" : "!r",
			msg->payload);

	// TODO: parse payload
	jmsg = cJSON_Parse(msg->payload);
	key = cJSON_GetObjectItemCaseSensitive(jmsg, "node");
	loggerf(INFO, "In on_message(): node = %s\n", key->valuestring);
	if (strcmp(key->valuestring, "polyglot") != 0) {
		loggerf(INFO, "-- Not from polyglot, ignoring %s\n", key->valuestring);
		return;
	}

	// What other objects can be in the message?
	//   connected
	//   config
	//   shortPoll
	//   longPoll
	if (cJSON_HasObjectItem(jmsg, "connected")) {
		/* call start callback */
		loggerf(INFO, "In on_message(): connected, calling start() callback\n");
		
		if (p->start) {
			int ret;
			ret = pthread_create(&thread, NULL, p->start, NULL);
		}
	} else if (cJSON_HasObjectItem(jmsg, "config")) {
		/* store config object and call onConfig */
		key = cJSON_GetObjectItem(jmsg, "config");
		loggerf(INFO, "config object = %s\n", cJSON_Print(key));
		// do we need to strdup this string?
		p->config = cJSON_Print(key);
		if (p->onConfig) {
			pthread_create(&thread, NULL, p->onConfig, (void *)p->config);
		}
	} else if (cJSON_HasObjectItem(jmsg, "shortPoll")) {
		logger(INFO, "In on_message(): payload = shortPoll\n");
		if (p->shortPoll)
			pthread_create(&thread, NULL, p->shortPoll, NULL);
	} else if (cJSON_HasObjectItem(jmsg, "longPoll")) {
		logger(INFO, "In on_message(): payload = longPoll\n");
		if (p->longPoll)
			pthread_create(&thread, NULL, p->longPoll, NULL);
	} else {
		logger(INFO, "Message type not yet handled\n");
	}

	logger(INFO, "on_message() finished.\n");
}

static void on_subscribe(struct mosquitto *m, void *ptr, int mid, int qos, const int *granted)
{
	logger(INFO, "on_subscribe() called. Not used\n");
}

static void on_publish(struct mosquitto *m, void *ptr, int res)
{
	logger(INFO, "on_publish() called. Not used\n");
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
