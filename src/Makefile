SRC = ehcpi.c input.c
DEFINES = -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=600 -D_BSD_SOURCE
FLAGS_COMMON = -W -Wall -Wextra -std=c99 ${DEFINES}
LIBS = $(shell pkg-config --cflags --libs libevent)
DESTDIR := /usr

.PHONY: debug lint release

ehcpi: ${SRC}
	$(CC) ${FLAGS_COMMON} ${CFLAGS} ${LIBS} ${SRC} -ggdb -O0 -o ehcpi
release: ${SRC}
	$(CC) ${FLAGS_COMMON} ${CFLAGS} ${LIBS} ${SRC} -DNDEBUG -o ehcpi

lint:
	clang-tidy ${SRC} -- ${DEFINES}
