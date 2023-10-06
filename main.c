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
#define LENGTH(X) (sizeof (X) / sizeof (X[0]))
#define CMDLENGTH 100UL

#include "dwmblocks.h"

int main(int argc, char **argv) {
    if ((dpy = XOpenDisplay(NULL)) == NULL) {
        fprintf(stderr, "Error opening X display\n");
        exit(EXIT_FAILURE);
    }
    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);

    status_loop();
}
