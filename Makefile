LIBS=-L/usr/local/lib -lmosquitto -lssl -lcrypto -lcares -lpthread
INCS=-I /usr/local/include
CFLAGS=$(INCS)

all: polyglot_mqtt.c cJSON.o
	cc -o polyglot_mqtt $(INCS) $(LIBS) polyglot_mqtt.c cJSON.o

.c.o:
	cc $(CFLAGS) -c -g $<
