#!/bin/sh

display() {
    printf "📋 "
    sleep 0.1
    result="$(timeout 0.5s xclip -selection clipboard -o -t TARGETS)"

    if echo "$result" | grep -q "image/png"; then
        image=true
        printf "PNG\n"
    elif [ -n "$result" ]; then
        content="$(timeout 1s dash -c "xclip -selection clipboard -o | head -n 100")"
        /usr/bin/echo "$content" \
            | sed -zE 's/^\s+//g;
                       s/([^^])\s+/\1 /g;
                       s/\s+$//g;
                       s/(.{14}\S).{4,}(\S.{5})/\1 ... \2/g;'
    fi
}

case "$BLOCK_BUTTON" in
    1) pgrep clipsim || setsid -f clipsim -d ;;
    2) clipsim --save  && dunstify "📋 clipsim" "History saved" ;;
    "") display ;;
    6) st -e vim "$0"                                           ;;
esac
