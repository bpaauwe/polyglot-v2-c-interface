LIBS=-L/usr/local/lib -lmosquitto -lssl -lcrypto -lcares -lpthread -lmarkdown -L ../ -lpolyglotiface
INCS=-I /usr/local/include -I ../
CFLAGS=$(INCS)


OBJS = TemplateController.o \
	   TemplateNode.o

CC = cc

template-poly: template-poly.c ../c_interface.h ../libpolyglotiface.so.1 $(OBJS)
	cc -g -o template-poly $(INCS) $(LIBS) template-poly.c $(OBJS)

ns_template: ns_template.c ../c_interface.h ../libpolyglotiface.so.1
	cc -g -o ns_template $(INCS) $(LIBS) ns_template.c

all: template-poly

clean:
	rm -f *.a *.so *.o *.core core
.c.o:
	cc $(CFLAGS) -c -g $<
