SRC = ehcpi.c input.c
DEFINES = -D_XOPEN_SOURCE=600 -D_POSIX_C_SOURCE=200809L
FLAGS_COMMON := -Wall -std=c99 ${DEFINES}

ehcpi: ${SRC}
	$(CC) ${FLAGS_COMMON} ${CFLAGS} ${SRC} -o ehcpi

.PHONY: install

install:
	install -d ${DESTDIR}${PREFIX}/bin
	install -m 755 ehcpi ${DESTDIR}${PREFIX}/bin
	install -m 755 init/openrc/ehcpi /etc/init.d/ehcpi

.PHONY: lint

lint:
	clang-tidy ${SRC} -- ${DEFINES}
