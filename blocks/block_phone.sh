#!/bin/sh

display () {
simple-mtpfs -l \
    | awk '
          !/Unknown/ {
              gsub("[1-9]:", "📱");
              if (NF > 1)
                  printf("\n%s %s", $1, $2) 
          }'
}

case $BLOCK_BUTTON in
    1) setsid -f android-file-transfer ;;
    6) "$TERMINAL" -e "$EDITOR" "$0"   ;;
    *) display ;;
esac 2> /dev/null
