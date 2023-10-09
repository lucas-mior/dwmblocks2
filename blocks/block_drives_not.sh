#!/bin/sh

display() {
    lsblk -nrpo "name,type,size,mountpoint" | \
        awk '
        BEGIN {
            i = 0;
        }
        NF == 3 && $2 == "part" {
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

case $BLOCK_BUTTON in
    6) setsid -f "$TERMINAL" -e vim "$0" ;;
    "") display ;;
esac 2> /dev/null
