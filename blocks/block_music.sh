#!/bin/sh

filter() {
    awk -v FS=" - " '
    NR == 1 { 
        song = $0; 
    }
    NR == 2 { 
       if ($0 ~ "playing")
           printf("🎵 %s\n", song);
    }'
}

case $BLOCK_BUTTON in
    1) album_art_play.sh toggle      | filter ;;
    2) album_art_play.sh toggle      | filter ;;
    3) pause.bash wall               | filter ;;
    6) "$TERMINAL" -e "$EDITOR" "$0"          ;;
    *) mpc status --format="%title%" | filter ;;
esac 2> /dev/null
