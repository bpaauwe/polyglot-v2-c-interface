
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
 * internal header for c_interface library.  These data structures and
 * functions are not visible to the application.
 */

#ifndef c_int_interface__h
#define c_int_interface__h

#ifdef __cplusplus
extern "C"
{
#endif

struct mqtt_priv {
	int profile_num;
	struct iface_ops *ns_ops;
	void *(*start)(void *args);
	void *(*longPoll)(void *args);
	void *(*shortPoll)(void *args);
	void *(*onConfig)(void *args);
};

struct profile {
	int num;
	char *config;
	int connected;
	int custom_config_doc_sent;
	struct mqtt_priv mqtt_info;
	struct node *nodelist;
};

void poly_send(cJSON *msg);
void *node_cmd_exec(void *args);
void *node_query_exec(void *args);
void *node_status_exec(void *args);

#ifdef __cplusplus
}
#endif

#endif
