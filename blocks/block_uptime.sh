#!/bin/sh

display () {
    uptime \
        | awk '{
            str = gensub("^([0-9])+$", "\\1m", "g", $3);
            str = gensub(",", "", "g", str);
            str = gensub(":", "h", "g", str);
            printf("🆙 %s\n", str);
        }'
}

case $1 in
    6) setsid -f "$TERMINAL" -e "$EDITOR" "$0" ;;
    *) display ;;
esac
