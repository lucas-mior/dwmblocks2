#!/usr/bin/awk -f

# usage: $0 /sys/class/power_supply/BAT0/uevent

BEGIN {
    FS = "="
}

/POWER_SUPPLY_STATUS/ {
    state = $2
}
/POWER_SUPPLY_CAPACITY=/ {
    capacity = $2
}

END {
    if (state == "Discharging") {
        if (capacity < 10) {
            printf("dunstify -r $DWMBLOCKS2_BATTERY        \
                  \"🪫 Bateria criticamente baixa (%d%%)\" \
                  \"Desligando...\"", capacity) | "/bin/dash";
            system("sync");
            system("sleep 5s");
            system("shutdown force");
        } else if (capacity < 20) {
            icon="🪫"
            printf("dunstify -r $DWMBLOCKS2_BATTERY        \
                  \"🪫 Bateria criticamente baixa (%d%%)\" \
                  \"Conectar carregador\"", capacity) | "/bin/dash";
        } else if (capacity < 40) {
            icon="🪫"
            printf("dunstify -t 1000 -r $DWMBLOCKS2_BATTERY \
                  \"🪫 Bateria baixa (%d%%)\"    \
                  \"Conectar carregador\"", capacity) | "/bin/dash";
        } else {
            icon="🔋"
        }
    } else {
        if (state ~ "(Full|Charging)") {
            icon="🔌 "
        } else {
            icon="🔌❓"
        }
    }
    printf("%s%d%%\n", icon, capacity);
}
