#ifndef BLOCKS_H
#define BLOCKS_H

static Block blocks[] = {
/* command                  update interval   update signal */
{"block_recording.sh",                    0,  10},
{"block_clipboard.sh",                    0,  11},
{"block_ytdlp.sh",                        0,  12},
{"block_drives_not.sh",                   0,  13},
{"block_drives_mount.sh",                 0,  14},
{"block_phone.sh",                        0,  15},
{"block_music.sh",                        0,  16},
{"block_volume.sh",                       0,  17},
{"block_microphone.sh",                   0,  18},
{"block_brilho.sh",                       0,  19},
//{"block_news.sh", 0, 20},
{"block_memory.sh",                      60,  22},
{"block_temperature.sh",                 60,  23},
{"block_internet.sh",                     0,  27},
{"block_bluetooth.sh",                   80,  25},
{"block_bateria.sh",                     60,  26},
};

#endif
