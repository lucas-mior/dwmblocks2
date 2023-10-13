#!/bin/sh

display () {
    sensors -u \
        | awk '
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
}

explain() {
    hogs="$(ps axch -o cmd:15,%cpu --sort=-%cpu | head)\\n(100% per core)"
    dunstify -r $DWMBLOCKS2_CPU " CPU hogs" "$hogs"
}

case $1 in
    1) explain ;;
    2) setsid -f "$TERMINAL" -e htop ;;
    3) explain ;;
    6) setsid -f "$TERMINAL" -e "$EDITOR" "$0" ;;
    *) display ;;
esac
