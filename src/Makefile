SRC = ehcpi.c input.c
CFLAGS += -std=c99 -D_XOPEN_SOURCE=600 -D_POSIX_C_SOURCE=200809L

ehcpi: ${SRC}
	$(CC) ${CFLAGS} ${SRC} -o ehcpi

.PHONY: install

install:
	install -d ${DESTDIR}${PREFIX}/bin
	install -m 755 ehcpi ${DESTDIR}${PREFIX}/bin
