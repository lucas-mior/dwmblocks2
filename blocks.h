#ifndef BLOCKS_H
#define BLOCKS_H

#include "dwmblocks2.h"
#include "block_functions.h"

Block blocks[] = {
/* function | command           signal environment variable |interval */
{NULL, "block_recording.sh",    "DWMBLOCKS2_RECORD",          0},
{NULL, "block_clipboard.sh",    "DWMBLOCKS2_CLIPBOARD",       0},
{NULL, "block_ytdlp.sh",        "DWMBLOCKS2_YOUTUBE",         0},
{NULL, "block_drives_not.sh",   "DWMBLOCKS2_DRIVES0",         0},
{NULL, "block_drives_mount.sh", "DWMBLOCKS2_DRIVES1",         0},
{NULL, "block_phone.sh",        "DWMBLOCKS2_DRIVES2",         0},
{NULL, "block_music.sh",        "DWMBLOCKS2_MUSIC",           0},
{NULL, "block_volume.sh",       "DWMBLOCKS2_VOLUME",          0},
{NULL, "block_microphone.sh",   "DWMBLOCKS2_MICROPHONE",      0},
{NULL, "block_bright.sh",       "DWMBLOCKS2_BRIGHT",          0},
{NULL, "block_memory.sh",       "DWMBLOCKS2_RAM",             0},
{NULL, "block_temperature.sh",  "DWMBLOCKS2_CPU",            60},
{NULL, "block_network.sh",      "DWMBLOCKS2_NETWORK",         0},
{NULL, "block_bluetooth.sh",    "DWMBLOCKS2_BLUETOOTH",      60},
{block_clock, NULL,             "DWMBLOCKS2_CLOCK",           1},
{NULL, "block_battery.sh",      "DWMBLOCKS2_BATTERY",        60},
{NULL, "block_uptime.sh",       "DWMBLOCKS2_UPTIME",         60},
};

#endif
