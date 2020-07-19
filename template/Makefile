LIBS=-L/usr/local/lib -lmosquitto -lssl -lcrypto -lcares -lpthread -lmarkdown -L ../ -lpolyglotiface
INCS=-I /usr/local/include -I ../
CFLAGS=$(INCS)


CC = cc

ns_template: ns_template.c ../c_interface.h ../libpolyglotiface.so.1
	cc -g -o ns_template $(INCS) $(LIBS) ns_template.c


clean:
	rm -f *.a *.so *.o *.core core
.c.o:
	cc $(CFLAGS) -c -g $<
