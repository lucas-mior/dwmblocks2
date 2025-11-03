/*
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

#if !defined(BLOCK_FUNCTIONS_C)
#define BLOCK_FUNCTIONS_C

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dwmblocks2.h"

static void
block_clock(int button, Block *block) {
    static bool show_epoch = false;
    if (block) {
        time_t seconds_since_epoch;
        struct tm t;
        char *string = block->output + 1;
        size_t n;
        seconds_since_epoch = time(NULL);
        localtime_r(&seconds_since_epoch, &t);

        // TODO: use async-safe strftime
        if (button == 7) {
            show_epoch = !show_epoch;
        }
        if (show_epoch) {
            n = snprintf(string, MAX_BLOCK_OUTPUT_LENGTH - 1, "📅 %ld ",
                         seconds_since_epoch);
        } else {
            n = strftime(string, MAX_BLOCK_OUTPUT_LENGTH - 1, "📅 %a %d/%m %T ",
                         &t);
        }
        block->length = (int)n + 1;
    }

    switch (button) {
    case 1:
        if (fork() == 0) {
            execlp("sh", "sh", "-c",
                   "yad --calendar --date-format='%A, %x'"
                   " --undecorated --fixed --no-buttons "
                   "| tr -d '\n' | xsel -b",
                   NULL);
            exit(EXIT_SUCCESS);
        }
        break;
    case 2:
        if (fork() == 0) {
            execlp("printscreen.sh", "printscreen.sh", "tela", NULL);
            exit(EXIT_SUCCESS);
        }
        break;
    case 3:
        if (fork() == 0) {
            execlp("killall", "killall", "yad", NULL);
            exit(EXIT_SUCCESS);
        }
        break;
    case 6:
        if (fork() == 0) {
            execlp("st", "st", "-e", "vim", __FILE__, NULL);
            exit(EXIT_SUCCESS);
        }
        break;
    default:
        break;
    }
    return;
}

#if __INCLUDE_LEVEL__ == 0
#include <assert.h>

int
main(void) {
    assert(true);
    exit(EXIT_SUCCESS);
}

#endif

#endif
