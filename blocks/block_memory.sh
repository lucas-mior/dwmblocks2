#!/bin/sh

case $DWMBLOCKS2_BUTTON in
    1) dunstify -r "$RAM" "🧠 Memory" "$(ps axch -o cmd:15,%mem --sort=-%mem | head)" & ;;
    2) setsid -f st -e htop                                                           & ;;
    3) dunstify -r "$RAM" "🧠 Memory" "$(ps axch -o cmd:15,%mem --sort=-%mem | head)" & ;;
    6) "$TERMINAL" -e "$EDITOR" "$0"                                                  & ;;
esac > /dev/null 2> /dev/null

free | awk '
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
