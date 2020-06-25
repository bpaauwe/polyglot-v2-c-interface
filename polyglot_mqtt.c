#include <stdio.h>
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

int main (int argc, char **argv)
{
	int ret;

	/* Create runtime instance with random client ID */
	/*  client name, true, priv_data */
	mosq = mosquitto_new(NULL, true, NULL);
	if (!mosq) {
		fprintf(stderr, "Failed to initialize a MQTT instance.\n");
		return -1;
	}

	/* Set callbacks */
	mosquitto_connect_callback_set(mosq, on_connect);
	mosquitto_message_callback_set(mosq, on_message);

	//mosquitto_username_pw_set (mosq, cfg->name, cfg->pass);
	//
	//ret = mosquitto_connect(mosq, "mqtts://polisy.bobsplace.com", 1883, 0);
	ret = mosquitto_connect(mosq, "mqtts://192.168.92.11", 1883, 0);
	if (ret) {
		fprintf (stderr, "Can't connect to Mosquitto broker %s %d\n",
				"mqtts://polisy.bobsplace.com", ret);
		return -1;
	}

	// TODO: subscribe to something here?

	ret = mosquitto_loop_forever(mosq, 1000, 1000);
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
}

static void on_connect(struct mosquitto *m, void *ptr, int res)
{
	printf("Connected.\n");
}

static void on_message(struct mosquitto *m, void *ptr,
		const struct mosquitto_message *msg)
{
	if (msg == NULL)
		return;

	printf("-- got message @ %s: (%d, Qos %d, %s) '%s'\n",
			msg->topic, msg->payloadlen, msg->qos, msg->retain ? "R" : "!r",
			msg->payload);
}
