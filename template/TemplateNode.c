#include <stdio.h>
#include <stdlib.h>
#include <c_interface.h>

/*
 * We'd like to be able to override the node's function table to
 * point to our own functions.
 *
 * First we need to add a few functions to the node function table.
 *   - start
 *   - longpoll
 *   - shortpoll
 *   - query
 */

/*
 * Optional.
 * This method is run once the Node is successfully added to the ISY
 * and we get a return result from Polyglot. Only happens once.
 */
static void start(struct node *self)
{
	loggerf(DEBUG, "%s: get ST=%s\n", self->name, self->ops.getDriver(self, "ST"));
	self->ops.setDriver(self, "ST", "1", 1, 1, 2);
	loggerf(DEBUG, "%s: get ST=%s\n", self->name, self->ops.getDriver(self, "ST"));
	self->ops.setDriver(self, "ST", "0", 1, 1, 2);
	loggerf(DEBUG, "%s: get ST=%s\n", self->name, self->ops.getDriver(self, "ST"));
	self->ops.setDriver(self, "ST", "1", 1, 1, 2);
	loggerf(DEBUG, "%s: get ST=%s\n", self->name, self->ops.getDriver(self, "ST"));
	self->ops.setDriver(self, "ST", "0", 1, 1, 2);
	loggerf(DEBUG, "%s: get ST=%s\n", self->name, self->ops.getDriver(self, "ST"));
}

/*
 */
static void short_poll(struct node *self)
{
	logger(DEBUG, "shortPoll\n");

	if (atoi(self->ops.getDriver(self, "ST")) == 1)
		self->ops.setDriver(self, "ST", "0", 1, 1, 2);
	else
		self->ops.setDriver(self, "ST", "1", 1, 1, 2);

	loggerf(DEBUG, "%s: get ST=%s\n", self->name, self->ops.getDriver(self, "ST"));
}

static void long_poll(struct node *self)
{
	logger(DEBUG, "longPoll\n");
}

/*
 * Called by ISY to report all drivers for this node. This is done in
 * the parent class, so you don't need to override this method unless
 * there is a need.
 */
static void query(struct node *self)
{
	self->ops.reportDrivers(self);
}

/*
 * Example command received from ISY.
 * set DON on TemplateNode.
 * Set the ST (status) driver to 1 or "true"
 */
static void cmd_on(struct node *self, char *id, char *value, int uom)
{
	self->ops.setDriver(self, "ST", "1", 1, 1, 2);
}

/*
 * Example command received from ISY.
 * set DOF on TemplateNode.
 * Set the ST (status) driver to 0 or "false"
 */
static void cmd_off(struct node *self, char *id, char *value, int uom)
{
	self->ops.setDriver(self, "ST", "0", 1, 1, 2);
}

/*
 * Not really a ping, but don't care... It's an example.
 */
static void cmd_ping(struct node *self, char *id, char *value, int uom)
{
	loggerf(DEBUG, "cmd_ping: Enter: id = %s value = %s\n", id, value);
}


/*
 * Create a Template Node structure
 */
struct node *TemplateNode(char *address, char *parent, char *name)
{
	struct node *n;

	n = allocNode("templatenodeid", address, parent, name);
	if (n == NULL)
		return n;

	/*
	 * Create an array containing the variable names(drivers), values and
	 * uoms(units of measure) from ISY. This is how ISY knows what kind
	 * of variable to display. Check the UOM's in the WSDK for a complete list.
	 * UOM 2 is boolean so the ISY will display 'True/False'
	 */
	addDriver(n, "ST", "1", 2);

	/*
	 * Create an array of commands. If ISY sends a command to the NodeServer,
	 * this tells it which method to call. DON calls setOn, etc.
	 */
	addCommand(n, "DON", cmd_on);
	addCommand(n, "DOF", cmd_off);
	addCommand(n, "PING", cmd_ping);

	/*
	 * When a node is allocation it has a default set of node operations
	 * pre-defined. If we want to override those, we can do so here.
	 *
	 * In this case, override the start, shortPoll, and longPoll node
	 * operations.
	 */
	setNodeStart(n, start);
	setNodeLongPoll(n, long_poll);
	setNodeShortPoll(n, short_poll);
	setNodeQuery(n, query);

	/* Set the node's hint.  The hint is a way to represent the node type. */
	setNodeHint(n, '1', '2', '3', '4');

	return n;
}
