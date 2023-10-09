#!/bin/sh

network.awk /sys/class/net/w*/flags /sys/class/net/*/operstate

case $DWMBLOCKS2_BUTTON in
    1) nmcli radio wifi on   ;;
    2) setsid -f st -e nmtui ;;
    3) nmcli radio wifi off  ;;
    6) setsid -f "$TERMINAL" -e "$EDITOR" "$0" ;;
esac
