#include "dwmblocks.h"
#include "blocks.h"

int main(void) {
    int common_interval = -1;
    int seconds = -1;
    struct timespec sleep_time;
    struct timespec to_sleep;
    int screen;

    struct sigaction signal_action;
    struct sigaction signal_child_action;
    signal_child_action.sa_handler = SIG_DFL;
    signal_child_action.sa_flags = SA_NOCLDWAIT;

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

    signal_action.sa_sigaction = button_handler;
    signal_action.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR1, &signal_action, NULL);
    sigaction(SIGCHLD, &signal_child_action, NULL);

    for (uint i = 0; i < LENGTH(blocks); i += 1) {
        if (blocks[i].interval)
            common_interval = gcd(common_interval, blocks[i].interval);
    }
    sleep_time.tv_sec = common_interval;
    sleep_time.tv_nsec = 0;

    do {
        to_sleep = sleep_time;
        get_block_outputs(seconds);
        set_root(false);

        while (nanosleep(&to_sleep, &to_sleep) < 0);
        seconds += common_interval;
    } while (true);
}
