#!/bin/sh

display () {
    lsblk -nrpo "name,type,size,mountpoint" \
        | awk '
        BEGIN {
            i = 0;
        }
        NF >= 4 {
            drive = $NF;
            if (substr(drive, length(drive), 1) ~ "[GT]") {
                arr[i] = $NF
                i++ ;
            }
        }
        END {
            for (j = 0; j < i; j++)
                printf("💽 %s ", arr[j]);
            if (i > 0)
                printf("\n")
        }'
}

case $1 in
    6) setsid -f "$TERMINAL" -e "$EDITOR" "$0" ;;
    *) display ;;
esac
