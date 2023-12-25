PREFIX ?= /usr/local

.PHONY: all clean install uninstall release debug clang
.SUFFIXES:
.SUFFIXES: .c .o

all: release

CFLAGS += -std=c99 -D_DEFAULT_SOURCE
CFLAGS += -Wextra -Wall -Wno-missing-field-initializers
LDFLAGS += -lX11

clang: CC=clang
clang: clean release
clang: CFLAGS += -Weverything -Wno-unsafe-buffer-usage -Wno-disabled-macro-expansion -Wno-format-nonliteral

release: CFLAGS += -O2 -flto
release: dwmblocks2

debug: CFLAGS += -g -fsanitize=undefined
debug: clean dwmblocks2

src = block_functions.c main.c util.c
headers = dwmblocks2.h blocks.h block_functions.h

dwmblocks2: $(src) $(headers) Makefile
	ctags --kinds-C=+l *.h *.c
	vtags.sed tags > .tags.vim
	$(CC) $(CFLAGS) -o $@ $(src) $(LDFLAGS)

install: all
	install -Dm755 dwmblocks2 $(DESTDIR)$(PREFIX)/bin/dwmblocks2
	install -Dm644 dwmblocks2.1 $(DESTDIR)$(PREFIX)/man/man1/dwmblocks2.1

uninstall: all
	rm -f $(DESTDIR)$(PREFIX)/bin/dwmblocks2
	rm -f $(DESTDIR)$(PREFIX)/man/man1/dwmblocks2.1

clean:
	rm -rf ./dwmblocks2
