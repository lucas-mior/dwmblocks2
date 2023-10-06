.POSIX:

PREFIX = /usr/local
CC = clang

dwmblocks: dwmblocks.o
	$(CC) -D_DEFAULT_SOURCE -Weverything -Wno-unsafe-buffer-usage dwmblocks.o -lX11 -o dwmblocks
dwmblocks.o: dwmblocks.c config.h dwmblocks.h
	$(CC) -D_DEFAULT_SOURCE -c -Weverything -Wno-unsafe-buffer-usage dwmblocks.c
clean:
	rm -f *.o *.gch dwmblocks
install: dwmblocks
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f dwmblocks $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/dwmblocks
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/dwmblocks

.PHONY: clean install uninstall
