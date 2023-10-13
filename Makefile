PREFIX ?= /usr/local

objs = block_functions.o util.o

.PHONY: all clean install uninstall release debug clang
.SUFFIXES:
.SUFFIXES: .c .o

all: release

CFLAGS += -std=c99 -D_DEFAULT_SOURCE
CFLAGS += -Wextra -Wall -Wno-missing-field-initializers
LDFLAGS += -lX11

clang: CC=clang
clang: clean
clang: CFLAGS += -Weverything -Wno-unsafe-buffer-usage -Wno-disabled-macro-expansion
clang: release

release: CFLAGS += -O2
release: dwmblocks2

debug: CFLAGS += -g -fsanitize=undefined
debug: clean
debug: dwmblocks2

$(objs): Makefile dwmblocks2.h blocks.h block_functions.h

dwmblocks2: $(objs) main.c 
	ctags --kinds-C=+l *.h *.c
	vtags.sed tags > .tags.vim
	$(CC) $(CFLAGS) -o $@ $(objs) main.c $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

install: all
	install -Dm755 dwmblocks2 $(DESTDIR)$(PREFIX)/bin/dwmblocks2

uninstall: all
	rm -f $(DESTDIR)$(PREFIX)/bin/dwmblocks2

clean:
	rm -rf ./dwmblocks2 *.o
