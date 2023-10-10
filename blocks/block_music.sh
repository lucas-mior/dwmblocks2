#!/bin/sh

display() {
    mpc status --format="%title%" \
    | awk -v FS=" - " '
    NR == 1 { 
        song = $0; 
    }
    NR == 2 { 
       if ($0 ~ "playing")
           printf("🎵 %s\n", song);
    }'
}

case $DWMBLOCKS2_BUTTON in
    1) album_art_play.sh toggle ;;
    2) album_art_play.sh toggle ;;
    3) pause.bash wall          ;;
    6) setsid -f "$TERMINAL" -e "$EDITOR" "$0" ;;
    "") display ;;
esac 2> /dev/null
