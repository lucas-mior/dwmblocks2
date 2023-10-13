#!/bin/sh

display () {
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
    1|5) bright --less ;;
    2) red.sh        ;;
    3|4) bright --more ;;
    6) setsid -f "$TERMINAL" -e "$EDITOR" "$0" ;;
    *) display ;;
esac
