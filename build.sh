#!/bin/sh

# shellcheck disable=SC2086

target="${1:-build}"
PREFIX="${PREFIX:-/usr/local}"
DESTDIR="${DESTDIR:-/}"

main="main.c"
program="dwmblocks2"

CFLAGS="$CFLAGS -std=c11 -D_DEFAULT_SOURCE"
CFLAGS="$CFLAGS -Wextra -Wall"
CFLAGS="$CFLAGS -Wno-unused-macros -Wno-unused-function"
CFLAGS="$CFLAGS -Wno-missing-field-initializers"
CFLAGS="$CFLAGS -Wno-constant-logical-operand"
LDFLAGS="$LDFLAGS -lm $(pkg-config x11 --libs)"

CC=${CC:-cc}
if [ "$CC" = "clang" ]; then
    CFLAGS="$CFLAGS -Weverything"
    CFLAGS="$CFLAGS -Wno-unsafe-buffer-usage"
    CFLAGS="$CFLAGS -Wno-format-nonliteral"
    CFLAGS="$CFLAGS -Wno-format-pedantic"
    CFLAGS="$CFLAGS -Wno-pre-c11-compat"
    CFLAGS="$CFLAGS -Wno-c++-keyword"
    CFLAGS="$CFLAGS -Wno-covered-switch-default"
    CFLAGS="$CFLAGS -Wno-implicit-void-ptr-cast"
    CFLAGS="$CFLAGS -Wno-disabled-macro-expansion "
fi

if [ "$target" = "debug" ]; then
    CFLAGS="$CFLAGS -g -fsanitize=undefined -DDWMBLOCKS2_DEBUG=1"
else
    CFLAGS="$CFLAGS -O2 -flto -DDWMBLOCKS2_DEBUG=0"
fi

case "$target" in
"uninstall")
    set -x
    rm -f "${DESTDIR}${PREFIX}/bin/${program}"
    rm -f "${DESTDIR}${PREFIX}/man/man1/${program}.1"
    ;;
"install")
    [ ! -f $program ] && $0 build
    set -x
    install -Dm755 "${program}"  "${DESTDIR}${PREFIX}/bin/${program}"
    install -Dm644 "${program}.1" "${DESTDIR}${PREFIX}/man/man1/${program}.1"
    ;;
"build"|"debug")
    ctags --kinds-C=+l -- *.h *.c 2> /dev/null || true
    vtags.sed tags > .tags.vim 2> /dev/null || true
    set -x
    $CC $CFLAGS -o ${program} "$main" $LDFLAGS
    ;;
*)
    echo "usage: $0 [ uninstall / install / build / debug ]"
    ;;
esac
