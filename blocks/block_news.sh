#!/bin/sh

# Displays number of unread news items and an loading icon if updating.
# When clicked, brings up `newsboat`.

case $DWMBLOCKS2_BUTTON in
    1) setsid -f st -e newsboat      ;;
    2) setsid -f newsup.sh           ;;
    6) "$TERMINAL" -e "$EDITOR" "$0" ;;
    *) ;;
esac > /dev/null 2> /dev/null

newsboat -x print-unread | awk '{if ($1 > 0) print " " $1}'
