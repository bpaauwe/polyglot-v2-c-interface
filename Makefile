LIBS=-L/usr/local/lib -lmosquitto -lssl -lcrypto -lcares -lpthread
INCS=-I /usr/local/include
CFLAGS=$(INCS)

OBJS=polyglot_mqtt.o pg_c_logger.o pg_c_interface.o pg_c_nodes.o cJSON.o

all: ns_template.c c_interface.h c_int_interface.h $(OBJS)
	cc -o ns_template $(INCS) $(LIBS) ns_template.c $(OBJS)

.c.o:
	cc $(CFLAGS) -c -g $<
