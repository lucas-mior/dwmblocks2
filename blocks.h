#ifndef BLOCKS_H
#define BLOCKS_H

static Block blocks[] = {
/* command                  update interval   update signal */
{"block_recording.sh",                    0,  1},
{"block_clipboard.sh",                    0,  2},
{"block_ytdlp.sh",                        0,  3},
{"block_drives_not.sh",                   0,  4},
{"block_drives_mount.sh",                 0,  5},
{"block_phone.sh",                        0,  6},
{"block_music.sh",                        0,  7},
{"block_volume.sh",                       0,  8},
{"block_microphone.sh",                   0,  9},
{"block_brilho.sh",                       0, 10},
//{"block_news.sh", 0, 20},
{"block_memory.sh",                      60, 11},
{"block_temperature.sh",                 60, 12},
{"block_internet.sh",                     0, 13},
{"block_bluetooth.sh",                   80, 14},
{"block_bateria.sh",                     60, 15},
};

#endif
