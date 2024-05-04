#ifndef BLOCK_FUNCTIONS_C
#define BLOCK_FUNCTIONS_C

#include <locale.h>

#include "dwmblocks2.h"

#pragma push_macro("TESTING_THIS_FILE")
#define TESTING_THIS_FILE 0

#pragma pop_macro("TESTING_THIS_FILE")

static void
block_clock(int button, Block *block) {
    if (block) {
        time_t seconds_since_epoch;
        struct tm t;
        char *string = block->output + 1;
        size_t n;
        seconds_since_epoch = time(NULL);
        localtime_r(&seconds_since_epoch, &t);

        // TODO: use async-safe strftime
        n = strftime(string, MAX_BLOCK_OUTPUT_LENGTH - 1,
                     "📅 %a %d/%m %T ", &t);
        block->length = (int) n + 1;
    }

    switch (button) {
    case 1:
        if (fork() == 0) {
            execlp("sh", "sh", "-c",
                   "yad --calendar --date-format='%A, %x'"
				   " --undecorated --fixed --no-buttons "
                   "| tr -d '\n' | xsel -b");
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
    default:
        break;
    }
    return;
}

#ifndef TESTING_THIS_FILE
#define TESTING_THIS_FILE 0
#endif

#if TESTING_THIS_FILE
#include <assert.h>

int
main(void) {
    assert(true);
    exit(EXIT_SUCCESS);
}

#endif

#endif
