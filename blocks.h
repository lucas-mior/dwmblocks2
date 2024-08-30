/* This file is part of dwmblocks2.
 * Copyright (C) 2024 Lucas Mior

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.

 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BLOCKS_H
#define BLOCKS_H

#include "dwmblocks2.h"
#include "block_functions.c"

Block blocks[] = {
/* function | command           signal environment variable |interval */
{NULL, "block_recording.sh",    "DWMBLOCKS2_RECORD",          0},
{NULL, "block_clipboard.sh",    "DWMBLOCKS2_CLIPBOARD",       0},
{NULL, "block_volume.sh",       "DWMBLOCKS2_VOLUME",          0},
{NULL, "block_microphone.sh",   "DWMBLOCKS2_MICROPHONE",      0},
{NULL, "block_bright.sh",       "DWMBLOCKS2_BRIGHT",          0},
{NULL, "block_network.sh",      "DWMBLOCKS2_NETWORK",         0},
{block_clock, NULL,             "DWMBLOCKS2_CLOCK",           60},
{NULL, "block_battery.sh",      "DWMBLOCKS2_BATTERY",        60},
{NULL, "block_uptime.sh",       "DWMBLOCKS2_UPTIME",         60},
{NULL, "block_ytdlp.sh",        "DWMBLOCKS2_YOUTUBE",         0},
{NULL, "block_drives_not.sh",   "DWMBLOCKS2_DRIVES0",         0},
{NULL, "block_trafic.sh",       "DWMBLOCKS2_TRAFIC",          2},
{NULL, "block_drives_mount.sh", "DWMBLOCKS2_DRIVES1",         0},
{NULL, "block_phone.sh",        "DWMBLOCKS2_DRIVES2",         0},
{NULL, "block_written.sh",      "DWMBLOCKS2_UPTIME",         10},
{NULL, "block_music.sh",        "DWMBLOCKS2_MUSIC",           0},
{NULL, "block_memory.sh",       "DWMBLOCKS2_RAM",             0},
{NULL, "block_temperature.sh",  "DWMBLOCKS2_CPU",            60},
{NULL, "block_mining.sh",       "DWMBLOCKS2_CPU",            10},
{NULL, "block_ip.sh",           "DWMBLOCKS2_NETWORK",        60},
{NULL, "block_bluetooth.sh",    "DWMBLOCKS2_BLUETOOTH",      60},
};

#endif
