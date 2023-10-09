#!/bin/sh

info="$(timeout 1s bluetoothctl show | awk '/Powered/{print $2}')"

if [ "$info" != "yes" ]; then
    echo " off"
else
    printf " "
    name="$(bluetoothctl devices Connected \
            | cut -d ' ' -f 3- | sed -E ':a;N;$!ba;s/\n/  /g;')"
    if [ "$name" != "$HOST" ]; then
        echo "$name"
    else
        echo "on"
    fi
fi

case $DWMBLOCKS2_BUTTON in
    1) bluetoothctl power on  && dunstify " on"  ;;
    3) bluetoothctl power off && dunstify " off" ;;
    6) "$TERMINAL" -e "$EDITOR" "$0" ;;
esac 2> /dev/null

