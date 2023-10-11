#!/bin/sh

## see: $HOME/.local/scripts/count_ydl.sh

cache="$HOME/.cache/count_ydl/"
cachedat="${cache}/dat"
cachevid="${cache}/vid"
cachedeu="${cache}/deu"
cacheeng="${cache}/eng"

dir="$XDG_VIDEOS_DIR/yt-dlp"
dirdeu="${dir}/0alemao/"
direng="${dir}/0ingles_adam/"

printf "▶ "
num="$(find "$dir" -type f | wc -l)"
numdeu="$(find -L "$dirdeu" -type f | wc -l)"
numeng="$(find -L "$direng" -type f | wc -l)"

ontem="$(cat "$cachedat")"
ontemvid="$(cat "$cachevid")"
ontemdeu="$(cat "$cachedeu")"
ontemeng="$(cat "$cacheeng")"

printf "$num / $((num-ontemvid+10))" | tee "${cachevid}.h"
printf " / $((numdeu-ontemdeu+1))"  | tee "${cachedeu}.h"
echo   " / $((numeng-ontemeng+1))"  | tee "${cacheeng}.h"

case "$1" in
    1) setsid -f "$TERMINAL" -e lfimg.bash "$dir" ;;
    6) setsid -f "$TERMINAL" -e "$EDITOR" "$0"    ;;
esac
