#!/bin/sh

display () {
    uptime \
        | awk '{
            str = gensub(",", "", "g", $3);
            str = gensub(":", "h", "g", str);
            printf("🆙 %s\n", str);
        }'
}

case $1 in
    6) setsid -f $TERMINAL -e $EDITOR "$0" ;;
    *) display ;;
esac
