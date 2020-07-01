/*
 * pg_c_logger.c
 *
 * Logging functions.  This initializes a set of functions to log
 * messages to the node server's log/debug.log file.  If it is
 * not able to open the log file, messages will be sent to stderr
 * instead.
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

static FILE *log;
static int debuglog = 1;

void initialize_logging(void)
{
	// TODO: check if log directory exist? if not, create it
	// TODO: What about log rotation?
	log_level = INFO;

	log = fopen("debug.log", "a");
	if (log == NULL)
		fprintf(stderr, "Failed to open log file: debug.log (%d)\n", errno);
}

void logger(enum LOGLEVELS level, char *msg)
{
	if (log && (level <= log_level)) {
		fprintf(log, "%s", msg);
		fflush(log);
		if (debuglog)
			fprintf(stderr, "%s", msg);
	} else {
		fprintf(stderr, "%s", msg);
	}
}

void loggerf(enum LOGLEVELS level, char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	if (log && (level <= log_level)) {
		vfprintf(log, fmt, args);
		fflush(log);
	} else {
		vfprintf(stderr, fmt, args);
	}
	va_end(args);

	if (debuglog) {
		va_start(args, fmt);
		vfprintf(stderr, fmt, args);
		va_end(args);
	}
}
