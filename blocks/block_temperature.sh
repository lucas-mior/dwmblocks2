#!/bin/sh

case $BLOCK_BUTTON in
    1) dunstify -r $CPU " CPU hogs" "$(ps axch -o cmd:15,%cpu --sort=-%cpu | head)\\n(100% per core)" & ;;
    2) setsid -f st -e htop                                                                            & ;;
    3) dunstify -r $CPU " CPU hogs" "$(ps axch -o cmd:15,%cpu --sort=-%cpu | head)\\n(100% per core)" & ;;
    6) "$TERMINAL" -e "$EDITOR" "$0" ;;
esac > /dev/null 2> /dev/null

sensors -u | awk '
/Core/ {
    core = $2
    while ($0 !~ "input")
        getline
    temp = int($2)
    if (temp > maxtemp) {
        maxtemp = temp
        coremax = core
    }
}
/nvme/ {
    while ($0 !~ "input")
        getline
    tempn = int($2)
    if (tempn > 60)
        printf("🔥💽 %dºC ", tempn)
}
END {
    if (temp > 60)
        printf("🔥🧠 Core%s %dºC \n", coremax, maxtemp)
}'
