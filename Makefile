LIBS=-L/usr/local/lib -lmosquitto -lssl -lcrypto -lcares
INCS=-I /usr/local/include
CFLAGS=$(INCS) $(LIBS)

all: polyglot_mqtt.c
	cc -o polyglot_mqtt $(INCS) $(LIBS) polyglot_mqtt.c

.c.o:
	cc $(CFLAGS) -c -g $<
