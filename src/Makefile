SRC = ehcpi.c input.c
OBJ = ehcpi.o input.o
DEFINES = -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=600 -D_BSD_SOURCE
FLAGS = -Wall -Wextra -std=c99 ${DEFINES}
LIBS = $(shell pkg-config --cflags --libs libevent)

ehcpi: ${OBJ}
	$(CC) ${LIBS} ${OBJ} ${FLAGS} -ggdb -o ehcpi

ehcpi.o: ehcpi.c input.h pathnames.h
	$(CC) -c -o $@ ${FLAGS} ehcpi.c
input.o: input.c
	$(CC) -c -o $@ ${FLAGS} input.c

.PHONY: lint
lint:
	clang-tidy ${SRC} -- ${DEFINES}
