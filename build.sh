#!/bin/sh

testing () {
    for src in *.c; do
        [ "$src" = "$main" ] && continue
        printf "Testing $src...\n"

        flags="$(awk '/flags:/ { $1=$2=""; print $0 }' "$src")"
        set -x
        if $CC $CFLAGS -D TESTING_THIS_FILE=1 $src -o $src.exe $flags; then
            ./$src.exe
        else
            printf "Failed to compile ${RED} $src ${RES}, is main() defined?\n"
        fi

        set +x 
    done
    rm *.exe
}

target="${1:-build}"
PREFIX="${PREFIX:-/usr/local}"
DESTDIR="${DESTDIR:-/}"

main="main.c"
program="dwmblocks2"

CFLAGS="$CFLAGS -std=c99 -D_DEFAULT_SOURCE "
CFLAGS="$CFLAGS -Wextra -Wall -Wno-unused-macros -Wno-missing-field-initializers "
LDFLAGS="$LDFLAGS -lm $(pkg-config x11 --libs)"

CC=${CC:-cc}
if [ $CC = "clang" ]; then
    CFLAGS="$CFLAGS -Weverything "
    CFLAGS="$CFLAGS -Wno-unsafe-buffer-usage -Wno-format-nonliteral "
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
        rm -f ${DESTDIR}${PREFIX}/bin/${program}
        rm -f ${DESTDIR}${PREFIX}/man/man1/${program}.1
        ;;
    "test")
        testing
        ;;
    "install")
        [ ! -f $program ] && $0 build
        set -x
        install -Dm755 ${program} ${DESTDIR}${PREFIX}/bin/${program}
        install -Dm644 ${program}.1 ${DESTDIR}${PREFIX}/man/man1/${program}.1
        ;;
    "build"|"debug")
        ctags --kinds-C=+l *.h *.c 2> /dev/null || true
        vtags.sed tags > .tags.vim 2> /dev/null || true
        set -x
        $CC $CFLAGS -o ${program} "$main" $LDFLAGS
        ;;
    *)
        echo "usage: $0 [ uninstall / test / install / build / debug ]"
        ;;
esac
