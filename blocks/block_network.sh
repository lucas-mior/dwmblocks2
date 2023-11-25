#!/bin/sh

display () {
    network.awk /sys/class/net/w*/flags /sys/class/net/*/operstate
}

case $1 in
    1) nmcli radio wifi on   ; display ;;
    2) setsid -f st -e nmtui ; display ;;
    3) nmcli radio wifi off  ; display ;;
    6) setsid -f $TERMINAL -e $EDITOR "$0" ;;
    *) display ;;
esac
