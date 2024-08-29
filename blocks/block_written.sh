#!/bin/sh

awk '{
    suffixes[0] = "B";
    suffixes[1] = "kB";
    suffixes[2] = "MB";
    suffixes[3] = "GB";
    suffixes[4] = "TB";
    suffixes[5] = "PB";

    pretty = $7*512;
    i = 0;
    while (pretty > 1024) {
        pretty /= 1024;
        i += 1;
    }

    if (pretty >= 1000) {
        printf("💾 %.0f%s\n", pretty, suffixes[i]);
    } else if (pretty >= 100) {
        printf("💾 %.1f%s\n", pretty, suffixes[i]);
    } else if (pretty >= 10) {
        printf("💾 %.2f%s\n", pretty, suffixes[i]);
    } else {
        printf("💾 %.3f%s\n", pretty, suffixes[i]);
    }
}' /sys/block/nvme0n1/stat
