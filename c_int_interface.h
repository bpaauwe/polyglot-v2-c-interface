
/*
 * We're going to need a structure that holds pointers to all
 * kinds of information.  I think I need to split profile
 * up.
 */
struct mqtt_priv {
	int profile_num;
	void *(*start)(void *args);
	void *(*longPoll)(void *args);
	void *(*shortPoll)(void *args);
	void *(*onConfig)(void *args);
};

struct profile {
	int num;
	char *config;
	int connected;
	struct mqtt_priv mqtt_info;
	struct node *nodelist;
};

void poly_send(cJSON *msg);

