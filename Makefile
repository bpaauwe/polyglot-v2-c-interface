
SRCS = cJSON.c \
       pg_c_interface.c \
       pg_c_logger.c \
       pg_c_misc.c \
       pg_c_nodes.c \
       pg_c_notices.c \
       polyglot_mqtt.c

LIB = polyglotiface
SHLIB_MAJOR = 1
SHLIB_MINOR = 0
CFLAGS = -I /usr/local/include
MAN = libpolyglotiface.3

.include <bsd.lib.mk>

# .include <bsd.man.mk>
