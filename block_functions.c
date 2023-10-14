#include "dwmblocks2.h"
#include "block_functions.h"

void block_clock(int button, Output *out) {
    time_t seconds_since_epoch;
    struct tm t;
    char *week;
    char *week_names[] = { "dom", "seg", "ter", "qua", "qui", "sex", "sáb" };
    char *string = out->string + 1;
    int n;

    if (out) {
        seconds_since_epoch = time(NULL);
        t = *localtime(&seconds_since_epoch);
        week = week_names[t.tm_wday];

        n = snprintf(string, BLOCK_OUTPUT_LENGTH - 1,
                    "📅 %s %02d/%02d %02d:%02d:%02d ",
                     week, t.tm_mday, t.tm_mon + 1, t.tm_hour, t.tm_min, t.tm_sec);
        out->length = (uint32) n + 1;
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
