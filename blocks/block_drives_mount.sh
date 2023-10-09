#!/bin/sh

display() {
    lsblk -nrpo "name,type,size,mountpoint" \
        | awk '
          BEGIN {
              i = 0;
          }
          NF >= 4 && $4 !~ /\/boot|\/home$|SWAP|\/$/ {
              arr[i] = gensub("^.*/", "", "g", $NF);
              i++;
          }
          END {
              for(j = 0; j < i; j++) {
                  printf("💽 %s", arr[j]);
                  if (j < (i - 1))
                      printf(" ");
              }
              if (i > 0)
                  printf("\n")
          }'
}

case $DWMBLOCKS2_BUTTON in
    6) setsid -f "$TERMINAL" -e "$EDITOR" "$0" ;;
    "") display ;;
esac 2> /dev/null
