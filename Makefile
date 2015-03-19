CFLAGS=-I/usr/local/include -O2 -std=c99 -Wall -pedantic -DDEBUG
LDFLAGS=-L/usr/local/lib -lxcb -lxcb-util

SRC = wm0.c window.c handlers.c
OBJ = ${SRC:.c=.o}

.c.o:
	cc -o $@ -c ${CFLAGS} $<

wm0: ${OBJ}
	cc -o $@ ${LDFLAGS} ${OBJ}

clean:
	@rm -f wm0 ${OBJ}

.PHONY: clean
