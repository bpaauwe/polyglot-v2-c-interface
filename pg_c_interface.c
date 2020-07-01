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
