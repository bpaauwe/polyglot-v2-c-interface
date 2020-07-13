/*
 * Node server template/test program to demonstrate how to use the
 * Polygot C interface library.
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

static int configured = 0;

void *start(void *args)
{
	struct node *n;

	logger(INFO, "In Node server start function. Do something here.\n");

	n = allocNode("node_id", "primary", "address", "my_node");
	addDriver(n, "ST", "1", 2);
	addDriver(n, "GV0", "34", 14);
	addDriver(n, "GV1", "0",  17);
	addNode(n);

	n = allocNode("node_id", "primary", "address2", "my_2nd_node");
	addDriver(n, "ST", "1", 2);
	addNode(n);

	delNode("address");

	addNotice("test", "Notice test. this is a new notice");

	return NULL;
}

void *short_poll(void)
{
	struct pair *p, *t;
	logger(INFO, "In Node server short poll function. Get custom parameters.\n");

	p = getCustomParams();
	t = p;
	while (t) {
		loggerf(INFO, "Parameters: %s = %s\n", t->key, t->value);
		t = t->next;
	}
	freeCustomPairs(p);
	// TODO: free p

	return NULL;
}

void *long_poll(void)
{
	char *cfg; 

	logger(INFO, "In Node server long poll function. Do something here.\n");
	if (isConnected())
		logger(INFO, "Connected to Polyglot\n");
	else
		logger(INFO, "Not connected to Polyglot\n");

	cfg = getConfig();
	loggerf(INFO, "dump current config = %s\n", cfg);
	free(cfg);

	cfg = getCustomParam("api_key");
	if (cfg) {
		loggerf(INFO, "Found APIKey = %s\n", cfg);
		free(cfg);
	}

	removeNoticesAll();
	return NULL;
}

void *config(void *args)
{
	cJSON *cfg;

	logger(INFO, "In Node server config handler.\n");

	cfg = cJSON_Parse((char *)args);

	loggerf(INFO, "config: %s\n", cJSON_Print(cfg));

	configured = 1;

	return NULL;
}


int main (int argc, char **argv)
{
	int ret;
	struct pair p, p1;
	int do_once = 0;

	ret = init(start, short_poll, long_poll, config);

	loggerf(INFO, "init returned %d\n", ret);
	logger(INFO, "Test C node server starting\n");

	p.key = "api_key";
	p.value = "set me";
	p.next = NULL;

	p1.key = "delete me";
	p1.value = "Test deletion of this key";
	p1.next = NULL;
		

	/* For initial test, just wait here */
	logger(INFO, "Node server main code starts here\n");
	while(1) {
		sleep(1);
		if (configured && !do_once) {
			addCustomParams(&p);
			addCustomParams(&p1);
			do_once = 1;
			sleep(2);
			removeCustomParam("delete me");
		}
	}
}


