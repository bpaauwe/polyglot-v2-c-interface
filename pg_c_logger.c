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

/*
 * Initialize the logging capability. 
 *
 * The log is currently written to a file called debug.log. 
 * By default, the log level is initialized to INFO.
 *
 * TODO:
 *    The log file should go in a directory logs. Need to 
 *    make sure the directory exists first and if not create it.
 *
 *    A new log should get created at the start of each day (localtime)
 */
void initialize_logging(void)
{
	log_level = INFO;

	log = fopen("debug.log", "a");
	if (log == NULL)
		fprintf(stderr, "Failed to open log file: debug.log (%d)\n", errno);
}

/*
 * Output a simple log message to the log. The message is only
 * output if the messages level is at or below the currently set
 * log_level.
 */
void logger(enum LOGLEVELS level, const char *msg)
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

/*
 * Output a formatted log message to the log. The message is only
 * output if the messages level is at or below the currently set
 * log_level.
 */
void loggerf(enum LOGLEVELS level, const char *fmt, ...)
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

/*
 * Change the log_level to a new level.
 */
void logger_set_level(enum LOGLEVELS new_level)
{
	log_level = new_level;
}
