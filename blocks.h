#ifndef BLOCKS_H
#define BLOCKS_H

static Block blocks[] = {
/* command                signal environment variable interval, signal number (ignored) */
{"block_recording.sh",    "BLOCK_RECORD",             0,        0},
{"block_clipboard.sh",    "BLOCK_CLIPBOARD",          0,        0},
{"block_ytdlp.sh",        "BLOCK_YOUTUBE",            0,        0},
{"block_drives_not.sh",   "BLOCK_DRIVES0",            0,        0},
{"block_drives_mount.sh", "BLOCK_DRIVES1",            0,        0},
{"block_phone.sh",        "BLOCK_DRIVES2",            0,        0},
{"block_music.sh",        "BLOCK_MUSIC",              0,        0},
{"block_volume.sh",       "BLOCK_VOLUME",             0,        0},
{"block_microphone.sh",   "BLOCK_MICROPHONE",         0,        0},
{"block_bright.sh",       "BLOCK_BRIGHT",             0,        0},
{"block_memory.sh",       "BLOCK_RAM",                0,        0},
{"block_temperature.sh",  "BLOCK_CPU",               60,        0},
{"block_network.sh",      "BLOCK_NETWORK",            0,        0},
{"block_bluetooth.sh",    "BLOCK_BLUETOOTH",         60,        0},
{"block_battery.sh",      "BLOCK_BATTERY",           60,        0},
};

#endif
