#!/bin/bash

display () {

    pulse="$(pacmd list-sources | sed -E -n '/\* index/,/index/p')"

    [[ "$pulse"  =~ state:\ (SUSPENDED|IDLE) ]] && exit

    awk '/active port/ { sub("<analog-input-headphone-mic>", "🎙")
                         sub("<analog-input-headset-mic>",   "🎙")
                         sub("<analog-input-internal-mic>",  "🎤")
                         sub("<analog-input-mic>",           "🎤")
                         sub("<analog-input-[^>]+>",         "🎤")
                         qual = $3 }
         /muted/       { muted = $2 }
         /^\s+volume:/ { volume = $5 }
         END           { if (muted == "yes") 
                             printf("🙊 0%%\n")
                         else
                             printf("%s %s", qual, volume)      }' <<< "$pulse"
    pacmd list-modules | sed -nE '/module-loopback/{s/.+/ 🔁/g;p}'
}

case $DWMBLOCKS2_BUTTON in
    1) vol_in.bash toggle    ; display ;;
    2) audio_toggle.bash     ; display ;;
    3) st -e pulsemixer      ; display ;;
    4) vol_in.bash up        ; display ;;
    5) vol_in.bash down      ; display ;;
    6) "$TERMINAL" -e "$EDITOR" "$0" ;;
    *)                         display ;;
esac 2> /dev/null
