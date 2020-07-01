LIBS=-L/usr/local/lib -lmosquitto -lssl -lcrypto -lcares -lpthread
INCS=-I /usr/local/include
CFLAGS=$(INCS)

all: ns_template.c c_interface.h polyglot_mqtt.o pg_c_logger.o cJSON.o
	cc -o ns_template $(INCS) $(LIBS) ns_template.c polyglot_mqtt.o pg_c_logger.o cJSON.o

.c.o:
	cc $(CFLAGS) -c -g $<
