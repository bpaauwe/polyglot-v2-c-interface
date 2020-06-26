#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <mosquitto.h>

struct mosquitto *mosq = NULL;

static void on_connect(struct mosquitto *m, void *ptr, int res);
static void on_message(struct mosquitto *m, void *ptr,
		const struct mosquitto_message *msg);
int init();

int main (int argc, char **argv)
{
	char *line = NULL;
	size_t linecap = 0;
	ssize_t linelen;

	while ((linelen = getline(&line, &linecap, stdin)) > 0)
		fwrite(line, linelen, 1, stdout);
	free(line);

	init();

	/* For initial test, just wait here */
	printf("Node server main code starts here\n");
	while(1)
		sleep(1);

	//mosquitto_destroy(mosq);
	//mosquitto_lib_cleanup();
}

struct profile {
	int num;
};

/*
 * Initialize the link to Polyglot
 */
int init(void)
{
	char msg[50];
	int ret;
	struct profile *p;

	p = calloc(sizeof(p), 1);
	p->num = 2;  // TODO: get this from stdin?

	/* Create runtime instance with random client ID */
	/*  client name, true, priv_data */
	mosq = mosquitto_new(NULL, true, (void *)p);
	if (!mosq) {
		fprintf(stderr, "Failed to initialize a MQTT instance.\n");
		return -1;
	}

	/* Set callbacks */
	mosquitto_connect_callback_set(mosq, on_connect);
	mosquitto_message_callback_set(mosq, on_message);

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
	fprintf(stderr, "tls set returned %d\n", ret);
	mosquitto_tls_opts_set(mosq, 0, NULL, NULL);

	/* Make the connection */
	/* TODO: Host and port should come from stdin */
	ret = mosquitto_connect_async(mosq, "192.168.92.11", 1883, 0);
	switch (ret) {
		case MOSQ_ERR_SUCCESS:
			fprintf(stderr, "successful connection.\n");
			break;
		case MOSQ_ERR_ERRNO:
			fprintf(stderr, "can't connect: %s\n", strerror(errno));
			return -1;
		case MOSQ_ERR_INVAL:
			fprintf(stderr, "can't connect: invalid arguments\n");
			return -1;
		default:
			fprintf(stderr, "can't connect: %s (%d)\n", mosquitto_strerror(ret), ret);
			return -1;
	}

	/* Let polyglot know we're connected. */
	/* TODO: profile number needs to come from stdin */
	ret = mosquitto_publish(mosq, NULL, "udi/polyglot/connections/polyglot", strlen(msg), msg, 0, 0);

	/* Start a thread to monitor the connection */
	ret = mosquitto_loop_start(mosq);
	//ret = mosquitto_loop_forever(mosq, 1000, 1000);

	return 0;
}


/*
 * Once the connection is established, subscribe to the necessary topics
 *
 * TODO: Need to have the profile number pulled from ptr.
 */
#define POLYGLOT_TOPIC  "udi/polyglot/connections/polyglot"
#define POLYGLOT_NS_TOPIC "udi/polyglot/connections/%d"
#define NODESERVER_TOPIC "udi/polyglot/ns/%d"

static void on_connect(struct mosquitto *m, void *ptr, int res)
{
	char msg[30];
	char topic[30];
	int ret;
	struct profile *p = (struct profile *)ptr;

	printf("Subscribing to node server profile %d\n", p->num);

	ret = mosquitto_subscribe(m, NULL, POLYGLOT_TOPIC, 0);
	fprintf(stderr, "subscribe returned %d\n", ret);

	sprintf(topic, POLYGLOT_NS_TOPIC, p->num);
	mosquitto_subscribe(m, NULL, topic, 0);

	sprintf(topic, NODESERVER_TOPIC, p->num);
	mosquitto_subscribe(m, NULL, topic, 0);

	/* publish a message to kick things off */
	// { node: this.profile, connected: true }
	sprintf(msg, "{node: %d, connected: true}", p->num);
	ret = mosquitto_publish(m, NULL, topic, strlen(msg), msg, 0, 0);
	fprintf(stderr, "publish returned %d\n", ret);
}

static void on_message(struct mosquitto *m, void *ptr,
		const struct mosquitto_message *msg)
{
	if (msg == NULL) {
		printf("-- got NULL message\n");
		return;
	}

	printf("-- got message @ %s: (%d, Qos %d, %s) '%s'\n",
			msg->topic, msg->payloadlen, msg->qos, msg->retain ? "R" : "!r",
			msg->payload);
}
