LIBS=-L/usr/local/lib -lmosquitto -lssl -lcrypto -lcares -lpthread -lmarkdown -L ../ -lpolyglotiface
INCS=-I /usr/local/include -I ../
CFLAGS=$(INCS)


OBJS = TemplateController.o \
	   TemplateNode.o

CC = cc

template-poly: template-poly.c ../c_interface.h ../libpolyglotiface.so.1 $(OBJS)
	cc -g -o template-poly $(INCS) $(LIBS) template-poly.c $(OBJS)

all: template-poly

clean:
	rm -f *.a *.so *.o *.core core
.c.o:
	cc $(CFLAGS) -c -g $<
