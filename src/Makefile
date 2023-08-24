SRC = ehcpi.c input.c
DEFINES = -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=600 -D_BSD_SOURCE
FLAGS_COMMON := -Wall -std=c99 ${DEFINES}

.PHONY: debug install lint

ehcpi: ${SRC}
	$(CC) ${FLAGS_COMMON} ${CFLAGS} ${LIBS} ${SRC} -o ehcpi

debug:
	$(CC) ${FLAGS_COMMON} ${CFLAGS} ${LIBS} ${SRC} -ggdb -o ehcpi

install:
	install -d ${DESTDIR}${PREFIX}/bin
	install -m 755 ehcpi ${DESTDIR}${PREFIX}/bin
	install -m 755 init/openrc/ehcpi /etc/init.d/ehcpi

lint:
	clang-tidy ${SRC} -- ${DEFINES}
