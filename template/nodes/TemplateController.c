#include <c_interface.c>

static void update_profile(struct node *n, char *id, char *value, int uom);
static void cmd_set_debug_mode(struct node *n, char *id, char *value, int uom);
static void remove_notices_all(struct node *n, char *id, char *value, int uom);
static void remove_notice_test(struct node *n, char *id, char *value, int uom);
static void cmd_query(struct node *n, char *id, char *value, int uom);
static void cmd_discover(struct node *n, char *id, char *value, int uom);

/*
 * The Controller interface defines the primary node from the ISY prespective.
 *
 * start() - Once the node server config is received from Polyglot, this
 * function is automatically called.  This is where you should do your
 * initial setup.
 *
 * long_poll() - Called every longPoll seconds. Set initially in the
 * server.json.
 *
 * short_poll() - Called every shortPoll seconds. Set initially in the
 * server.json.
 *
 * config() - Called whenever the node server configuration is changed.
 *
 * See the libpolyglotiface man page for a full list of functions available.
 */

static void *start(void *args)
{
	struct node *controller;
	/*
	 * Polyglot v2 Interface startup done. Here is where you start your
	 * integration.  This will run, once the NodeServer connects to Polyglot
	 * and gets it's config.
	 *
	 * In this example I am calling a discovery function. While this is
	 * optional, this is a good place to connfigure all the non-controller
	 * nodes.
	 */

	/* Create and configure the controller node */
	controller = allocNode("controller", "c_address", "c_address",
			"Template Controller");
	addDriver(controller, "ST", "1", 2);
	addDriver(controller, "GV1", "30", 25);
	addCommand(controller, "QUERY", cmd_query);
	addCommand(controller, "DISCOVER", cmd_discover);
	addCommand(controller, "UPDATE_PROFILE", update_profile);
	addCommand(controller, "REMOVE_NOTICES_ALL", remove_notices_all);
	addCommand(controller, "REMOVE_NOTICE_TEST", remove_notice_test);
	addCommand(controller, "SET_DM", cmd_set_debug_mode);
	addNode(controller);

	check_params();
	discover(controller->address);
}

static void *long_poll(void)
{
	struct node *n;

	/*
	 * This runs every 30 seconds. You would probably update your nodes either
	 * here or shortPoll.  The timer can be overriden in the server.json.
	 */

	logger(DEBUG, "long_poll\n");

	/* Get the node list and walk it calling a function to update the node */
	n = getNodes();
	while (n) {
		if (strcmp(n->address, controller_address) != 0)
			node_update(n)
	}
}

static void *short_poll(void)
{
	struct node *n;

	/*
	 * This runs every 10 seconds. You would probably update your nodes either
	 * here or longPoll.  The timer can be overriden in the server.json.
	 */
	logger(DEBUG, "short_poll\n");

	/* Get the node list and walk it calling a function to update the node */
	n = getNodes();
	while (n) {
		if (strcmp(n->address, controller_address) != 0)
			node_update(n)
	}
}

void *query(void)
{
	struct node *n;

	/*
	 * By default a query to the control node reports the FULL driver set
	 * for ALL nodes back to ISY.
	 */
	n = getNodes();
	while (n) {
		n->report_drivers(n);
	}
}

void *stop(void)
{
	logger(DEBUG, "NodeServer stopped.\n");
}

static void *config(void *args)
{
	/*
	 * Called whenever the node server configuration changes. The new
	 * configuration is passed to this function.  The configuration
	 * is represented by a JSON string.
	 *
	 * To make use of this, a JSON parser is needed to parse the string
	 * into objects of some type.
	 */
	logger(DEBUG, "In Node server config handler.\n");
}

struct iface_ops controller_ops {
	.start = start;
	.shortPoll = short_poll;
	.longPoll = long_poll;
	.onConfig = config;
};


/*
 * Example
 * Do discovery here. Does not have to be called discovery. Called from
 * example controller start method and from DISCOVER command recieved from
 * ISY as an exmaple.
 */
static void discover(void)
{
	addNode(TemplateNode(self.address, 'templateaddr', 'Template Node Name'))
}


static void check_params(void)
{
	char *default_user = "YourUserName";
	char *default_password = "YourPassword";
	char *user, *pass;
	struct pair *u, *p;
	/*
	 * this is an example of using custom params for username, password, and
	 * structure (JSON) data.
	 */
	removeNoticesAll();
	addNotice("example", "Hello Friends!");

	if ((user = getCustomParam("user")) != NULL) {
		loggerf(DEBUG, "Found user name = %s\n", user);
	} else {
		user = default_user;
		loggerf(ERROR, "check_params: username not defined in customParams, please add it.\n");
	}


	if ((pass = getCustomParam("password")) != NULL) {
		loggerf(DEBUG, "Found password = %s\n", pass);
	} else {
		pass = default_password;
		loggerf(ERROR, "check_params: password not defined in customParams, please add it.\n");
	}

	/* Make sure they are in the params */
	p = malloc(sizeof(struct pair));
	p->key = "password";
	p->value = pass;
	p->next = NULL;
	u = malloc(sizeof(struct pair));
	u->key = "user";
	u->value = user;
	u->next = p;
	addCustomParams(u);

	free(p);
	free(u);

	/* Add a notice to change the user/password from the default */
	if (strcmp(user, default_user) == 0 ||
			strcmp(pass, default_password) == 0)
		addNotice("test", "Please set proper user and password in configuration page and restart this node server.");
}

static void update_profile(struct node *self, char *id, char *value, int uom)
{
	logger(INFO, "Update Profile\n");
	installProfile();
}

static void remove_notices_all(struct node *self, char *id, char *value, int uom)
{
	logger(INFO, "remove all notices\n");
	removeNoticesAll();
}

static void remove_notice_test(struct node *self, char *id, char *value, int uom)
{
	logger(INFO, "remove only test notice\n");
	removeNotice("test");
}

static void cmd_query(struct node *self, char *id, char *value, int uom)
{
	logger(INFO, "Query command\n");
	query();
}

static void cmd_discover(struct node *self, char *id, char *value, int uom)
{
	logger(INFO, "Discover command\n");
	discover(self->address);
}

static void cmd_set_debug_mode(struct node *self, char *id, char *value, int uom)
{
	loogerf(DEBUG, "cmd_set_debug_mode: %s\n", value);

	/* FIXME: need to convert the value to enum */
	logger_set_level(atoi(value));
}

