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
    int interrupted = 0;
    int interval = -1;
    int idx = 0;
    struct timespec sleep_time;
	struct timespec to_sleep;
	int screen;

    struct sigaction sa;
	struct sigaction sigchld_action = {
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
            sigaddset(&sa.sa_mask, SIGRTMIN+blocks[i].signal);
        }
    }

    sa.sa_sigaction = button_handler;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGCHLD, &sigchld_action, NULL);

    for (uint i = 0; i < LENGTH(blocks); i += 1) {
        if (blocks[i].interval) {
            interval = greatest_common_denominator(blocks[i].interval, interval);
        }
    }
	sleep_time.tv_sec = interval;
	sleep_time.tv_nsec = 0;
    to_sleep = sleep_time;

    get_block_outputs(-1);
    while (true) {
        interrupted = nanosleep(&to_sleep, &to_sleep);
        if (interrupted == -1) {
			printf("dwmblocks is on interrupt loop!\n");
            continue;
        }
        get_block_outputs(idx);
        set_root();
        idx += interval;
        to_sleep = sleep_time;
    }
}
