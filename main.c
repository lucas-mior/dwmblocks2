#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "dwmblocks.h"
#include "blocks.h"

int main(void) {
    int64 seconds = -1;
    struct timespec sleep_time;
    struct timespec to_sleep;
    int screen;
    char *HORARIO;

    struct sigaction signal_action;
    struct sigaction signal_child_action;
    signal_child_action.sa_handler = SIG_DFL;
    signal_child_action.sa_flags = SA_NOCLDWAIT;

    if ((HORARIO = getenv("HORARIO")) == NULL) {
        fprintf(stderr, "HORARIO environmental variable is not defined\n.");
    }
    clock_signal = atoi(HORARIO);
    clock_output[0] = (char) clock_signal;

    if ((display = XOpenDisplay(NULL)) == NULL) {
        fprintf(stderr, "Error opening X display\n");
        exit(EXIT_FAILURE);
    }
    screen = DefaultScreen(display);
    root = RootWindow(display, screen);

    for (int i = SIGRTMIN; i <= SIGRTMAX; i += 1)
        signal(i, SIG_IGN);

    for (uint i = 0; i < LENGTH(blocks); i += 1) {
        if (blocks[i].signal > 0) {
            signal(SIGRTMIN + blocks[i].signal, signal_handler);
            sigaddset(&signal_action.sa_mask, SIGRTMIN + blocks[i].signal);
        }
    }
    signal(SIGRTMIN + clock_signal, signal_handler);
    sigaddset(&signal_action.sa_mask, SIGRTMIN + clock_signal);

    signal_action.sa_sigaction = button_handler;
    signal_action.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR1, &signal_action, NULL);
    sigaction(SIGCHLD, &signal_child_action, NULL);

    sleep_time.tv_sec = 1;
    sleep_time.tv_nsec = 0;

    do {
        to_sleep = sleep_time;
        get_block_outputs(seconds);
        set_root(false);

        while (nanosleep(&to_sleep, &to_sleep) < 0);
        seconds += 1;
    } while (true);
}
