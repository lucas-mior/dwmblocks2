#!/bin/sh

display () {
    free \
    | awk '
    NR == 2 || NR == 3 {
        max = strtonum($2)
        if ($3 > max*0.9) {
            printf("💽 %dM / 15G", $3/1024)
            system("dunstify \"System almost out of memory\"")
        } else if ($3 > max*0.75) {
            printf("💽 %dM / 15G", $3/1024)
        } else if ($3 > max*0.50) {
            printf("💽 %dM / 15G", $3/1024)
        } else if ($3 > max*0.25) {
            printf("💽 %dM / 15G", $3/1024)
        }
    }
    END {
        printf("\n");
    }'
}

explain () {
    memory_hugs="$(ps axch -o cmd:15,%mem --sort=-%mem | head)"
    dunstify -r "$DWMBLOCKS2_RAM" "🧠 Memory" "$memory_hugs"
}

case $1 in
    1) setsid -f "$TERMINAL" -e htop ;;
    3) explain                       ;;
    6) setsid -f "$TERMINAL" -e "$EDITOR" "$0" ;;
    "") display ;;
esac
