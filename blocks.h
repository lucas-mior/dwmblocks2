// SPDX-License-Identifier: AGPL
// Copyright (c) 2026 Lucas Mior

#if !defined(BLOCKS_H)
#define BLOCKS_H

#include "dwmblocks2.h"
#include "block_functions.c"

#if CC_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

Block blocks[] = {
/* function | command           signal environment variable |interval */
{NULL, "block_recording.sh",    "DWMBLOCKS2_RECORD",          0},
{NULL, "block_clipboard.sh",    "DWMBLOCKS2_CLIPBOARD",       0},
{NULL, "block_volume.sh",       "DWMBLOCKS2_VOLUME",         60},
{NULL, "block_microphone.sh",   "DWMBLOCKS2_MICROPHONE",     60},
{NULL, "block_bright.sh",       "DWMBLOCKS2_BRIGHT",         60},
{NULL, "block_network.sh",      "DWMBLOCKS2_NETWORK",        60},
{block_clock, NULL,             "DWMBLOCKS2_CLOCK",           1},
{NULL, "block_battery.sh",      "DWMBLOCKS2_BATTERY",        60},
{NULL, "block_uptime.sh",       "DWMBLOCKS2_UPTIME",         60},
{NULL, "block_ytdlp.sh",        "DWMBLOCKS2_YOUTUBE",        60},
{NULL, "block_drives_not.sh",   "DWMBLOCKS2_DRIVES0",        60},
{NULL, "block_trafic.sh",       "DWMBLOCKS2_TRAFIC",          2},
{NULL, "block_drives_mount.sh", "DWMBLOCKS2_DRIVES1",        60},
{NULL, "block_phone.sh",        "DWMBLOCKS2_DRIVES2",        60},
{NULL, "block_written.sh",      "DWMBLOCKS2_UPTIME",         10},
{NULL, "block_diskusage.sh",    "DWMBLOCKS2_DRIVES1",        60},
{NULL, "block_music.sh",        "DWMBLOCKS2_MUSIC",          60},
{NULL, "block_memory.sh",       "DWMBLOCKS2_RAM",            60},
{NULL, "block_temperature.sh",  "DWMBLOCKS2_CPU",            60},
{NULL, "block_mining.sh",       "DWMBLOCKS2_CPU",            10},
{NULL, "block_ip.sh",           "DWMBLOCKS2_NETWORK",        60},
{NULL, "block_bluetooth.sh",    "DWMBLOCKS2_BLUETOOTH",      60},
{NULL, "block_joystick.sh",     "DWMBLOCKS2_JOYSTICK",       60},
};

#if CC_CLANG
#pragma clang diagnostic pop
#endif

#endif
