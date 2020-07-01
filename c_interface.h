/*
 * Include file for the C Polyglot interface library.
 *
 * Contains all the function prototypes, structures, defines,
 * and enums that a Polyglot node server will need when using
 * this library.
 */

enum LOGLEVELS {
	CRITICAL,
	ERROR,
	WARNING,
	INFO,
	DEBUG,
};
enum LOGLEVELS log_level;
void logger(enum LOGLEVELS level, char *msg);
void loggerf(enum LOGLEVELS level, char *fmt, ...);

int init(void (*start), void (*shortPoll), void (*longPoll), void (*onConfig));
int isConnected(void);
char *getConfig(void);
