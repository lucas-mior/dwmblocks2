#!/bin/sh

display () {
    info="$(timeout 1s bluetoothctl show | awk '/Powered/{print $2}')"

    if [ "$info" != "yes" ]; then
        echo " off"
    else
        printf " "
        name="$(bluetoothctl devices Connected \
                | cut -d ' ' -f 3-4 | sed -E ':a;N;$!ba; s/\n/  /g;')"
        if [ "$name" != "$HOST" ]; then
            echo "$name"
        else
            echo "on"
        fi
    fi
}

power () {
    bluetoothctl power "$1"
}

case $1 in
    1) power on  > /dev/null ; display ;;
    3) power off > /dev/null ; display ;;
    6) setsid -f "$TERMINAL" -e "$EDITOR" "$0" ;;
    *) display ;;
esac
