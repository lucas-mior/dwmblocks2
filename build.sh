#!/bin/sh -e

# shellcheck disable=SC2086

dir=$(dirname "$(readlink -f "$0")")
cd "$dir" || exit

# shellcheck source=./cbase/common.sh
. "./cbase/common.sh"

program=$(common_get_program "$0")
script=$(basename "$0")
common_build_parse_args "$@"

case "$mode" in
build|check|debug|debug-fast|fast_feedback|install|test|uninstall)
    ;;
*)
    common_build_unknown_mode
    ;;
esac

common_build_print_invocation "$script"

PREFIX="${PREFIX:-/usr/local}"
DESTDIR="${DESTDIR:-/}"

exe="bin/$program"
mkdir -p "$(dirname "$exe")"

CC=$(common_get_compiler "$mode")

CPPFLAGS="$CPPFLAGS -Icbase"

CFLAGS="$CFLAGS -std=c11"
CFLAGS="$CFLAGS -Wfatal-errors"

LDFLAGS="$LDFLAGS -lm $(pkg-config x11 --libs)"

case "$mode" in
debug)
    CFLAGS="$CFLAGS -g3 -Og"
    CPPFLAGS="$CPPFLAGS -DDEBUGGING=1"
    ;;
debug-fast)
    CFLAGS="$CFLAGS -g2 -O2 -flto -march=native -ftree-vectorize"
    CFLAGS="$CFLAGS -fsanitize-trap=undefined"
    CPPFLAGS="$CPPFLAGS -DDEBUGGING=1"
    ;;
build)
    CFLAGS="$CFLAGS -O2 -flto -march=native -ftree-vectorize"
    ;;
fast_feedback)
    ;;
test|install|uninstall)
    ;;
build|check|debug|debug-fast|fast_feedback|install|test|uninstall)
    ;;
*)
    common_build_unknown_mode
    ;;
esac

case "$mode" in
uninstall)
    trace_on
    rm -f "${DESTDIR}${PREFIX}/bin/${program}"
    common_uninstall_opt "${program}.1" "${DESTDIR}${PREFIX}/man/man1/${program}.1"
    trace_off
    ;;
install)
    if [ ! -f "$exe" ]; then
        "$0" build
    fi
    trace_on
    install -Dm755 "$exe" "${DESTDIR}${PREFIX}/bin/${program}"
    common_install_opt -Dm644 "${program}.1" "${DESTDIR}${PREFIX}/man/man1/${program}.1"
    trace_off
    ;;
test)
    TEST_EXCLUDE_PATTERN='(^|/)cbase/' common_test "$target"
    exit
    ;;
check)
    (
        common_build_run_analyzers build
    )
    echo "static analysis finished."
    exit
    ;;
build|debug|debug-fast|fast_feedback)
    common_build_tags
    trace_on
    $CC $CPPFLAGS $CFLAGS -o "$exe" main.c $LDFLAGS
    trace_off
    ;;
esac
