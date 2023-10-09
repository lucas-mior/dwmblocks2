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
            printf("dunstify -r $BATERIA                   \
                  \"🪫 Bateria criticamente baixa (%d%%)\" \
                  \"Desligando...\"", capacity) | "/bin/dash";
            system("sync");
            system("sleep 5s");
            system("shutdown force");
        } else if (capacity < 20) {
            icon="🪫"
            printf("dunstify -r $BATERIA                   \
                  \"🪫 Bateria criticamente baixa (%d%%)\" \
                  \"Conectar carregador\"", capacity) | "/bin/dash";
        } else if (capacity < 40) {
            icon="🪫"
            printf("dunstify -t 1000 -r $BATERIA \
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

    switch (ENVIRON["DWMBLOCKS2_BUTTON"]) {
        case 6:
            file = ""
            cmd = "find ~/.local/scripts/ -name 'battery.awk' | head -n 1"
            while ((cmd | getline file) > 0);
            close(cmd)
            printf("setsid -f st -e vim %s", file) | "/bin/dash"
            break;
        case "":
            printf("%s%d%%\n", icon, capacity);
            break;
    }
}
