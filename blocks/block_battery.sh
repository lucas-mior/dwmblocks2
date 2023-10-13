#!/bin/sh

display () {
    battery.awk /sys/class/*/BAT0/uevent
}

case $1 in
    6) setsid -f "$TERMINAL" -e "$EDITOR" "$0" ;;
    *) display ;;
esac
