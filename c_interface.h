
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
 * Include file for the C Polyglot interface library.
 *
 * Contains all the function prototypes, structures, defines,
 * and enums that a Polyglot node server will need when using
 * this library.
 */

#ifndef c_interface__h
#define c_interface__h

#ifdef __cplusplus
extern "C"
{
#endif

enum LOGLEVELS {
	CRITICAL,
	ERROR,
	WARNING,
	INFO,
	DEBUG,
};
enum LOGLEVELS log_level;
void initialize_logging(void);
void logger(enum LOGLEVELS level, const char *msg);
void loggerf(enum LOGLEVELS level, const char *fmt, ...);
void logger_set_level(enum LOGLEVELS new_level);

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
	void (*callback)(char *cmd, char *value, int uom);
};

struct send {
	char *id;
	void (*callback)(char *cmd, char *value, int uom);
};

struct node;

struct node_ops {
	void (*setDriver)(struct node *n, char *driver, char *value, int report, int force, int uom);	
	void (*reportDriver)(struct node *n, char *driver, int changed, int force);	
	void (*reportDrivers)(struct node *n);	
	void (*reportCmd)(struct node *n, char *send, char *value, int uom);	
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
void addCommand(struct node *n, char *cmd_id, void (*callback)(char *, char *, int));
void addSend(struct node *n, char *cmd_id, void (*callback)(char *, char *, int));
void addNode(struct node *n);
void delNode(char *address);
struct node *getNode(char * address);
struct node *getNodes(void);
void addNotice(char *key, char *text);
void addNoticeTemp(char *key, char *text);
void removeNotice(char *key);
void removeNoticesAll(void);
struct pair *getNotices(void);
void setCustomParamsDoc(void);
void installProfile(void);
void restart(void);

#ifdef __cplusplus
}
#endif

#endif
