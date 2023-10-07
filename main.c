#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <X11/Xlib.h>

#include "dwmblocks.h"
#include "blocks.h"

int main(void) {
    int longest_common_interval = -1;
    int seconds = 0;
    struct timespec sleep_time;
    struct timespec to_sleep;
    int screen;

    struct sigaction signal_action;
    struct sigaction signal_child_action = {
        .sa_handler = SIG_DFL,
        .sa_flags = SA_NOCLDWAIT
    };

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
            signal(SIGRTMIN+blocks[i].signal, signal_handler);
            sigaddset(&signal_action.sa_mask, SIGRTMIN+blocks[i].signal);
        }
    }

    signal_action.sa_sigaction = button_handler;
    signal_action.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR1, &signal_action, NULL);
    sigaction(SIGCHLD, &signal_child_action, NULL);

    for (uint i = 0; i < LENGTH(blocks); i += 1) {
        if (blocks[i].interval) {
            longest_common_interval = gcd(blocks[i].interval, longest_common_interval);
        }
    }
    sleep_time.tv_sec = longest_common_interval;
    sleep_time.tv_nsec = 0;
    to_sleep = sleep_time;

    get_block_outputs(-1);
    while (true) {
        if (nanosleep(&to_sleep, &to_sleep) < 0)
            continue;
        get_block_outputs(seconds);
        set_root();
        seconds += longest_common_interval;
        to_sleep = sleep_time;
    }
}
