CFLAGS ?= -std=c99 -Wall -Wextra -Wpedantic -Werror -Wno-unused-parameter -Wno-unused-function -I/usr/include/readline
LDFLAGS ?= -lreadline

DEPS = debug.h list.h repl.h util.h
OBJS = main.o repl.o debug.o list.o

ddb: ${OBJS}
	${CC} -o $@ $^ ${LDFLAGS}

%.o: %.c ${DEPS}
	${CC} -c -o $@ $< ${CFLAGS}

clean:
	rm -f ddb *.o *.d

.PHONY: clean

