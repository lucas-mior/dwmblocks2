#!/bin/sh

display () {
    color=""
    if [ -e "$REDSHIFT_CACHE" ]; then
        red=$(cat "$REDSHIFT_CACHE")
        if [ $red -gt 5000 ]; then
            color="🟦"
        else
            color="🟥"
        fi
    fi
    bril="$(bright --print)"
    echo "$bril $color"
    dunstify -r $DWMBLOCKS2_BRIGHT "$bril" -t 500
}

case $1 in
    1|5) bright --less   ; display ;;
    2) red.sh >/dev/null ; display ;;
    3|4) bright --more   ; display ;;
    6) setsid -f "$TERMINAL" -e "$EDITOR" "$0" ;;
    *) display ;;
esac
