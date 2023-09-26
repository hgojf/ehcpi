all: 
	make -C src

install: all
	install -d ${DESTDIR}${PREFIX}/bin
	install -m 755 src/ehcpi ${DESTDIR}${PREFIX}/bin
	install -m 755 init/openrc/ehcpi /etc/init.d/ehcpi
