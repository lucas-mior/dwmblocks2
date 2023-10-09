#ifndef BLOCKS_H
#define BLOCKS_H

static Block blocks[] = {
/* command                signal environment variable interval, signal number (ignored) */
{"block_recording.sh",    "DWMBLOCKS2_RECORD",        0,        0},
{"block_clipboard.sh",    "DWMBLOCKS2_CLIPBOARD",     0,        0},
{"block_ytdlp.sh",        "DWMBLOCKS2_YOUTUBE",       0,        0},
{"block_drives_not.sh",   "DWMBLOCKS2_DRIVES0",       0,        0},
{"block_drives_mount.sh", "DWMBLOCKS2_DRIVES1",       0,        0},
{"block_phone.sh",        "DWMBLOCKS2_DRIVES2",       0,        0},
{"block_music.sh",        "DWMBLOCKS2_MUSIC",         0,        0},
{"block_volume.sh",       "DWMBLOCKS2_VOLUME",        0,        0},
{"block_microphone.sh",   "DWMBLOCKS2_MICROPHONE",    0,        0},
{"block_bright.sh",       "DWMBLOCKS2_BRIGHT",        0,        0},
{"block_memory.sh",       "DWMBLOCKS2_RAM",           0,        0},
{"block_temperature.sh",  "DWMBLOCKS2_CPU",          60,        0},
{"block_network.sh",      "DWMBLOCKS2_NETWORK",       0,        0},
{"block_bluetooth.sh",    "DWMBLOCKS2_BLUETOOTH",    60,        0},
{"block_battery.sh",      "DWMBLOCKS2_BATTERY",      60,        0},
};

#endif
