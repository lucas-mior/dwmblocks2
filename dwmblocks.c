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
#include "config.h"

Display *display;
int screen;
Window root;
char status_bar[LENGTH(blocks)][CMDLENGTH] = {0};
char statusstr[2][512];
static const char *delim = " ";

void remove_all(char *str, char to_remove) {
    char *read = str;
    char *write = str;
    while (*read) {
        if (*read != to_remove) {
            *write = *read;
			write += 1;
        }
        read += 1;
    }
    *write = '\0';
    return;
}

int greatest_common_denominator(int a, int b) {
    int temp;
    while (b > 0) {
        temp = a % b;
        a = b;
        b = temp;
    }
    return a;
}

void get_block_output(const Block *block, char *output) {
    char tmpstr[CMDLENGTH] = "";
	FILE *command_pipe;
    char *s;
    int e;
	int length;

    if (block->signal) {
        output[0] = (char) block->signal;
        output += 1;
    }
    if (!(command_pipe = popen(block->command, "r"))) {
        fprintf(stderr, "Failed to run %s: %s\n",
                         block->command, strerror(errno));
        return;
    }
    do {
        errno = 0;
        s = fgets(tmpstr, CMDLENGTH, command_pipe);
        e = errno;
    } while (!s && e == EINTR);
    pclose(command_pipe);

    strcpy(output, tmpstr);
    remove_all(output, '\n');

    length = (int) strlen(output);
    if ((length > 0 && block != &blocks[LENGTH(blocks) - 1])) {
        strcat(output, delim);
    }
    length += strlen(delim);
    output[length] = '\0';
    return;
}

void get_block_outputs(int time) {
    const Block *block;
    for (uint i = 0; i < LENGTH(blocks); i += 1) {
        block = blocks + i;
        if ((block->interval != 0 && time % block->interval == 0) || time == -1) {
            get_block_output(block, status_bar[i]);
        }
    }
    return;
}

void get_signal_commands(int signal) {
    const Block *block;
    for (uint i = 0; i < LENGTH(blocks); i += 1) {
        block = blocks + i;
        if (block->signal == signal) {
            get_block_output(block, status_bar[i]);
        }
    }
    return;
}

void setup_signals(void) {
    struct sigaction sa;
	struct sigaction sigchld_action = {
		.sa_handler = SIG_DFL,
		.sa_flags = SA_NOCLDWAIT
	};

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
	return;
}

int get_status(char *str, char *last) {
    strcpy(last, str);
    str[0] = '\0';
    for (uint i = 0; i < LENGTH(blocks); i += 1) {
        strcat(str, status_bar[i]);
        if (i == LENGTH(blocks) - 1)
            strcat(str, " ");
    }
    str[strlen(str)-1] = '\0';
    return strcmp(str, last);
}

void setroot(void) {
    if (!get_status(statusstr[0], statusstr[1]))
        return;
    XStoreName(display, root, statusstr[0]);
    XFlush(display);
    return;
}


void status_loop(void) {
    int interrupted = 0;
    int interval = -1;
    int idx = 0;
    struct timespec sleep_time;
	struct timespec to_sleep;

    setup_signals();
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
        setroot();
        idx += interval;
        to_sleep = sleep_time;
    }
}

void signal_handler(int signum) {
    get_signal_commands(signum - SIGRTMIN);
    setroot();
}

void button_handler(int sig, siginfo_t *si, void *ucontext) {
    char button[2] = {'0' + (si->si_value.sival_int & 7), '\0'};
	char shcmd[1024];
	char *command[4];
    pid_t process_id = getpid();
    (void) ucontext;
    sig = (si->si_value.sival_int >> 3) - 34;
    if (fork() == 0) {
        Block *block = NULL;
        for (uint i = 0; i < LENGTH(blocks); i += 1) {
            block = &blocks[i];
            if (block->signal == sig)
                break;
        }
        snprintf(shcmd, sizeof (shcmd),
				 "%s && kill -%d %d", block->command, block->signal+34, process_id);
        command[0] = "/bin/sh";
		command[1] = "-c";
		command[2] = shcmd;
		command[3] = NULL;
        setenv("BLOCK_BUTTON", button, 1);
        setsid();
        execvp(command[0], command);
        exit(EXIT_SUCCESS);
    }
}
