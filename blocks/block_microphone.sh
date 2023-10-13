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
}

case $1 in
    1) vol_in.bash toggle ;;
    2) audio_toggle.bash  ;;
    3) st -e pulsemixer   ;;
    4) vol_in.bash up     ;;
    5) vol_in.bash down   ;;
    6) setsid -f "$TERMINAL" -e "$EDITOR" "$0" ;;
    *) display ;;
esac
