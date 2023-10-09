#!/usr/bin/awk -f

BEGIN {
    ether = 0;
}

FILENAME ~ "net/e" {
    if ($1 ~ "up|unknown") {
        printf("🌐");
        ether = 1;
    }
}

FILENAME ~ "net/w.+flags" {
    wifi_on = $1 ~ "0x1003"
}

FILENAME ~ "net/w.+operstate" {
    if ($1 == "up") {
        printf("📶");
        while (("nmcli -t -f active,ssid dev wifi" | getline) > 0) {
            if ($0 ~ "sim") {
                name = substr($0, 5)
                printf(" %s", name)
            }
        }
    } else {
        if (wifi_on) {
            printf("📡");
        } else {
            if (!ether)
                printf("📶 off");
        }
    }

}

FILENAME ~ "net/ap" {
    if ($1 == "up")
        printf("📻");
}

FILENAME ~ "net/tun" {
    if ($1 == "up")
        printf("🔒");
}
