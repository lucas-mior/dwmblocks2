#include "dwmblocks2.h"
#include "block_functions.h"
#include "util.h"

void block_clock(int button, Block *block) {
    time_t seconds_since_epoch;
    struct tm t;
    char *week;
    char *week_names[] = { "dom", "seg", "ter", "qua", "qui", "sex", "sáb" };
    char *string = block->output + 1;
    int n;

    if (block) {
        seconds_since_epoch = time(NULL);
        localtime_r(&seconds_since_epoch, &t);
        week = week_names[t.tm_wday];

        n = snprintf(string, BLOCK_OUTPUT_LENGTH - 1,
                    "📅 %s %02d/%02d %02d:%02d:%02d ",
                     week, t.tm_mday, t.tm_mon + 1, t.tm_hour, t.tm_min, t.tm_sec);
        block->length = (uint32) n + 1;
    }

    switch (button) {
    case 1:
        if (fork() == 0) {
            execlp("sh", "sh", "-c",
                   "yad --calendar --undecorated --fixed --no-buttons "
                   "| tr -d '\n' | xsel -b");
            exit(EXIT_SUCCESS);
        }
        break;
    case 2:
        if (fork() == 0) {
            execlp("print_screen.sh", "print_screen.sh", "tela", NULL);
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
