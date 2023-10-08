PREFIX ?= /usr/local

.PHONY: all clean install uninstall release debug clang
.SUFFIXES:
.SUFFIXES: .c .o

all: release

CFLAGS += -std=c99 -D_DEFAULT_SOURCE
CFLAGS += -Wextra -Wall
LDFLAGS += -lX11

clang: CC=clang
clang: clean
clang: CFLAGS += -Weverything -Wno-unsafe-buffer-usage
clang: release

release: CFLAGS += -O2
release: dwmblocks

debug: CFLAGS += -g -fsanitize=undefined
debug: clean
debug: dwmblocks

dwmblocks: Makefile main.c dwmblocks.h blocks.h
	ctags --kinds-C=+l *.h *.c
	vtags.sed tags > .tags.vim
	$(CC) $(CFLAGS) -o $@ $(objs) main.c $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

install: all
	install -Dm755 dwmblocks $(DESTDIR)$(PREFIX)/bin/dwmblocks

uninstall: all
	rm -f $(DESTDIR)$(PREFIX)/bin/dwmblocks

clean:
	rm -rf ./dwmblocks *.o
