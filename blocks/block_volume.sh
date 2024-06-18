#!/bin/sh

display () {
    pacmd list-sinks | sed -E -n '/\* index/,/index/p' | \
        awk 'BEGIN         { FS="[ ,]+" }
             /active port/ { sub("<analog-output-headphones>","🎧")
                             sub("<analog-output-speaker>",   "🔊")
                             sub("<hdmi-output[^>]+>",        "📺")
                             sub("<headset-output>",          "🎧")
                             sub("<speaker-output>",          "🎧")
                             sub("<analog-output[^>]+>",      "🎧")
                             qual = $3 }
             /muted/       { muted = $2 }
             /^\s+volume:/ { volume = $5 }
             END           { if (muted == "yes") {
                                 qual   = "🔇"
                                 volume = "0%"
                                 printf("🔇 0%%\n")
                             } else {
                                 printf("%s %s\n", qual, volume)
                             }
                             if (strtonum(gensub("%", "", "g", volume)) > 100)
                                 printf("dunstify -r $DWMBLOCKS2_VOLUME -t 250 \"%s %s\"", qual, volume) | "/bin/dash"
             }'
}

case $1 in
    1) vol_out.sh toggle ; display ;;
    2) audio_toggle.bash ; display ;;
    3) st -e pulsemixer  ; display ;;
    4) vol_out.sh up     ; display ;;
    5) vol_out.sh down   ; display ;;
    6) setsid -f "$TERMINAL" -e "$EDITOR" "$0" ;;
    *) display ;;
esac
