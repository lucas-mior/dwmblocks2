// SPDX-License-Identifier: AGPL
// Copyright (c) 2026 Lucas Mior

#if !defined(BLOCK_FUNCTIONS_C)
#define BLOCK_FUNCTIONS_C

#include "cbase.h"
#include "dwmblocks2.h"

static void
block_clock(int button, Block *block) {
    static bool show_epoch = false;
    if (block) {
        time_t seconds_since_epoch;
        struct tm t;
        char *string = block->output + 1;
        int32 n;
        seconds_since_epoch = time(NULL);
        localtime_r(&seconds_since_epoch, &t);

        // TODO: use async-safe strftime
        if (button == 7) {
            show_epoch = !show_epoch;
        }
        if (show_epoch) {
            n = snprintf(string, MAX_BLOCK_OUTPUT_LENGTH - 1, "📅 %lld ",
                         (llong)seconds_since_epoch);
        } else {
            n = (int32)strftime(string, MAX_BLOCK_OUTPUT_LENGTH - 1,
                                "📅 %a %d/%m %T ", &t);
        }
        block->length = n + 1;
    }

    // TODO: The fork() calls below ignore fork failure, and a failed execlp()
    // exits with success instead of reporting the click action failure.
    switch (button) {
    case 1:
        if (fork() == 0) {
            execlp("sh", "sh", "-c",
                   "yad --calendar --date-format='%A, %x'"
                   " --undecorated --fixed --no-buttons "
                   "| tr -d '\n' | xsel -b",
                   NULL);
            exit(EXIT_FAILURE);
        }
        break;
    case 2:
        if (fork() == 0) {
            execlp("printscreen.sh", "printscreen.sh", "tela", NULL);
            exit(EXIT_FAILURE);
        }
        break;
    case 3:
        if (fork() == 0) {
            execlp("killall", "killall", "yad", NULL);
            exit(EXIT_FAILURE);
        }
        break;
    case 6:
        if (fork() == 0) {
            execlp("st", "st", "-e", "vim", __FILE__, NULL);
            exit(EXIT_FAILURE);
        }
        break;
    default:
        break;
    }
    return;
}

#endif
