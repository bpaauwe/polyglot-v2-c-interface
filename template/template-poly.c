#include <stdio.h>
#include <unistd.h>
/*
 * This is a NodeServer template for Polyglot v2 written in 'C'
 * by Robert Paauwe (bpaauwe@yahoo.com)
 *
 * Link to the libpolyglotiface library. The library should be installed
 * in /usr/local/lib on the Polisy.
 *
 * The include file c_interface.h provides the function prototypes and
 * required structure definitions.
 */

#include <c_interface.h>

/*
 * Prototypes for the callbacks that are defined in the controller
 * code.  See TemplateController.c
 */
extern struct iface_ops controller_ops;


int main (int argc, char **argv)
{
	int ret;
	struct pair p, p1;
	int do_once = 0;

	/*
	 * Initialize the interface library.
	 *
	 * This will start the communication with Polyglot. Pass in the
	 * functions that will provide your node server's implementation.
	 *   start()      - called after communication has been established.
	 *   config()     - called anytime the node server's configuration is
	 *                  changed.
	 *   short_poll() - called at the user defined short poll interval for
	 *                  the node server.
	 *   long_poll()  - called at the user defined long poll interval for
	 *                  the node server.
	 */
	ret = init(&controller_ops);

	/*
	 * The interface library contains a logging facility that is created
	 * by default and logs to "logs/debug.log"
	 *
	 * You can use  CRITICAL, ERROR, WARNING, INFO, DEBUG levels as needed.
	 */
	logger(INFO, "C Template node server starting\n");

	/* Wait here until stopped */
	while(1) {
		sleep(1);
	}

	logger(INFO, "C Template node server stopped\n");
}
