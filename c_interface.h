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

struct driver {
	char *driver;
	char *value;
	int uom;
};

struct command {
	char *id;
};

struct send {
	char *id;
};

struct node;

struct node_ops {
	void (*setDriver)(struct node *n, char *driver, char *value, int report, int force, int uom);	
	void (*reportDriver)(struct node *n, struct driver *d, int changed, int force);	
};


/*
 * node structure
 */
struct node {
	char *id;
	char *name;
	char *address;
	char *primary;
	int isPrimary;
	int enabled;
	int added;
	struct driver *drivers;
	int driver_cnt;
	struct command *commands;
	int command_cnt;
	struct send *sends;
	int send_cnt;
	unsigned int hint;
	struct node_ops ops;
	struct node *next;
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
struct node *allocNode(char *id, char *primary, char *address, char *name);
void addDriver(struct node *n, char *driver, char *init, int uom);
void addNode(struct node *n);
int delNode();
int getNode();
int getNodes();

