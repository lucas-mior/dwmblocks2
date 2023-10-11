#!/bin/sh

display () {
    network.awk /sys/class/net/w*/flags /sys/class/net/*/operstate
}

case $1 in
    1) nmcli radio wifi on   ;;
    2) setsid -f st -e nmtui ;;
    3) nmcli radio wifi off  ;;
    6) setsid -f "$TERMINAL" -e "$EDITOR" "$0" ;;
    "") display ;;
esac
