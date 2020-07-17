LIBS=-L/usr/local/lib -lmosquitto -lssl -lcrypto -lcares -lpthread -lmarkdown
INCS=-I /usr/local/include
CFLAGS=$(INCS)

# Shared library
PG_OBJS = polyglot_mqtt.o \
		  pg_c_logger.o \
		  pg_c_interface.o \
		  pg_c_nodes.o \
		  pg_c_notices.o \
		  pg_c_misc.o \
		  cJSON.o 
PG_INCS = c_interface.h \
		  c_int_interface.h
PG_LIB = libpolyglotiface.so
PICFLAGS = -fpic
DEFAULT_LIB_INSTALL_PATH = ./
SHARED_FLAGS = -shared -I /usr/local/include
SHARED_LDFLAGS = -Wl,-rpath,$(DEFAULT_LIB_INSTALL_PATH)
PG_LDFLAGS = -L$(DEFAULT_LIB_INSTALL_PATH) -lpolyglot

# Compiler and linker options
CC = cc
DEBUG_FLAGS = -g
WARN_FLAGS_BSD = -fno-strict-aliasing -pipe  -Wsystem-headers -Wall -Wno-format-y2k \
				 -W -Wno-unused-parameter -Wstrict-prototypes -Wmissing-prototypes \
				 -Wpointer-arith -Wreturn-type -Wcast-qual -Wwrite-strings -Wswitch \
				 -Wshadow -Wcast-align -Wunused-parameter -Wno-pointer-sign
OPT_FLAGS = -O2
LD_FLAGS = -lmosquitto -lssl -lcrypto -lcares -lpthread -lmarkdown
CFLAGS = $(DEBUG_FLAGS) $(OPT_FLAGS) $(WARN_FLAGS_BSD) $(PICFLAGS) $(INCS)

libpolyglotiface: $(PG_OBJS)
	$(CC) $(DEBUG_FLAGS) $(OPT_FLAGS) $(WARN_FLAGS_BSD) $(SHARED_FLAGS) \
		-o $(PG_LIB) $(PG_OBJS)

$(PG_OBJS): Makefile $(PG_INCS)

clean:
	rm -f *.a *.so *.o *.core core

dep: deps
deps:
	gcc -MM *.c

.PHONY: all clean

OBJS=polyglot_mqtt.o \
     pg_c_logger.o \
     pg_c_interface.o \
     pg_c_nodes.o \
     pg_c_notices.o \
     pg_c_misc.o \
     cJSON.o 

all: ns_template.c c_interface.h c_int_interface.h $(OBJS)
	cc -g -o ns_template $(INCS) $(LIBS) ns_template.c $(OBJS)

.c.o:
	cc $(CFLAGS) -c -g $<
