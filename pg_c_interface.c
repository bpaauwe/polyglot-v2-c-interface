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

extern struct mosquitto *mosq;
extern int polyglot_connected;

struct profile {
	int num;
	char *config;
	int junk;
	void *(*start)(void *args);
	void *(*longPoll)(void *args);
	void *(*shortPoll)(void *args);
	void *(*onConfig)(void *args);
};

int isConnected(void)
{
	return polyglot_connected;
}
