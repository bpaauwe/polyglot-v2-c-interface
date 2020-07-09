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

#define PARAMETER_CHANGED 0x01
struct pair {
	char *key;
	char *value;
	int flags;
	struct pair *next;
};

int init(void (*start), void (*shortPoll), void (*longPoll), void (*onConfig));
int isConnected(void);
char *getConfig(void);
struct pair *getCustomParams(void);
char *getCustomParam(char *key);
int saveCustomParams(struct pair *params);
int addCustomParams(struct pair *params);
int removeCustomParam(char *key);
char *getCustomData(char *key);
int saveCustomData(struct pair *params);
int addCustomData(struct pair *params);
int removeCustomData(char *key);
void freeCustomPairs(struct pair *params);


