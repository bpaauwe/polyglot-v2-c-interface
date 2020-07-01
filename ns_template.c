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

void *start(void *args)
{
	logger(INFO, "In Node server start function. Do something here.\n");
	return NULL;
}

void *short_poll(void)
{
	char *cfg; 

	logger(INFO, "In Node server short poll function. Do something here.\n");
	if (isConnected())
		logger(INFO, "Connected to Polyglot\n");
	else
		logger(INFO, "Not connected to Polyglot\n");


	cfg = getConfig();
	loggerf(INFO, "dump current config = %s\n", cfg);
	free(cfg);

	return NULL;
}

void *long_poll(void)
{
	logger(INFO, "In Node server long poll function. Do something here.\n");
	return NULL;
}

void *config(void *args)
{
	cJSON *cfg;

	logger(INFO, "In Node server config.\n");

	cfg = cJSON_Parse((char *)args);

	loggerf(INFO, "config: %s\n", cJSON_Print(cfg));

	return NULL;
}


int main (int argc, char **argv)
{
	int ret;

	ret = init(start, short_poll, long_poll, config);

	loggerf(INFO, "init returned %d\n", ret);
	logger(INFO, "Test C node server starting\n");

	/* For initial test, just wait here */
	logger(INFO, "Node server main code starts here\n");
	while(1)
		sleep(1);
}

