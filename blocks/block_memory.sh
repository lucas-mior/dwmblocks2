#!/bin/sh

display () {
    free \
    | awk '
    NR == 2 || NR == 3 {
        max = strtonum($2)
        max_pretty = max / (1024*1024);
        now = strtonum($3)
        now_pretty = now / (1024*1024);
        if (now > max*0.98) {
            system("killall dwm; killall sh");
        } else if (now > max*0.9) {
            printf("💽 %.1f/%.1fG", now_pretty, max_pretty)
            system("dunstify \"System almost out of memory\"")
        } else if (now > max*0.75) {
            printf("💽 %.1f/%.1fG", now_pretty, max_pretty)
        } else if (now > max*0.50) {
            printf("💽 %.1f/%.1fG", now_pretty, max_pretty)
        } else if (now > max*0.25) {
            printf("💽 %.1f/%.1fG", now_pretty, max_pretty)
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
    1) setsid -f "$TERMINAL" -e htop         ;;
    3) explain ; display ;;
    6) setsid -f "$TERMINAL" -e "$EDITOR" "$0" ;;
    *) display ;;
esac
