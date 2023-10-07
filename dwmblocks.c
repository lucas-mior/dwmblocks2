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

Display *display;
Window root;
static char status_bar[LENGTH(blocks)][BLOCK_OUTPUT_LENGTH] = {0};
static char status_new[sizeof (status_bar)];
static const char delim = ' ';

int gcd(int a, int b) {
    int temp;
    while (b > 0) {
        temp = a % b;
        a = b;
        b = temp;
    }
    return a;
}

void get_block_output(const Block *block, char *output) {
    char buffer[BLOCK_OUTPUT_LENGTH] = {0};
    FILE *command_pipe;
    char *status;
    int error;
    size_t length;
    Block *last_block = &blocks[LENGTH(blocks) - 1];

    if (block->signal) {
        output[0] = (char) block->signal;
        output += 1;
    }
    if (!(command_pipe = popen_no_shell(block->command))) {
        fprintf(stderr, "Failed to run %s: %s\n",
                         block->command, strerror(errno));
        return;
    }
    do {
        errno = 0;
        status = fgets(buffer, sizeof (buffer), command_pipe);
        error = errno;
    } while (!status && error == EINTR);
    // TODO: Check if pclose() is right here, because
    // popen_no_shell uses pipe() and fdopen()
    pclose(command_pipe);

    length = strcspn(buffer, "\n");
    buffer[length] = '\0';
    memcpy(output, buffer, length);
    if (length == 0)
        return;

    while (output[length - 1] == delim) {
        output[length - 1] = delim;
        length -= 1;
        if (length == 1)
            break;
    }
    if ((length > 0) && (block != last_block)) {
        output[length] = delim;
    }
    length += 1;
    output[length] = '\0';
    return;
}

void get_block_outputs(int seconds) {
    for (uint i = 0; i < LENGTH(blocks); i += 1) {
        Block *block = &blocks[i];
		if (seconds < 0) {
            get_block_output(block, status_bar[i]);
			continue;
		}
		if (block->interval == 0)
			continue;
        if ((seconds % block->interval) == 0)
            get_block_output(block, status_bar[i]);
    }
    return;
}

void set_root(void) {
    status_new[0] = '\0';
    for (uint i = 0; i < LENGTH(blocks); i += 1) {
        strcat(status_new, status_bar[i]);
        if (i == LENGTH(blocks) - 1)
            strcat(status_new, " ");
    }
    status_new[strlen(status_new) - 1] = '\0';

    XStoreName(display, root, status_new);
    XFlush(display);
    return;
}

void signal_handler(int signum) {
    for (uint i = 0; i < LENGTH(blocks); i += 1) {
        Block *block = &blocks[i];
        if (block->signal == (signum - SIGRTMIN)) {
            get_block_output(block, status_bar[i]);
        }
    }
    set_root();
}

void button_handler(int sig, siginfo_t *signal_info, void *ucontext) {
    char button[2] = {'0' + (signal_info->si_value.sival_int & 7), '\0'};
    char kill_command[1024];
    char *command[4];
    pid_t process_id = getpid();
    (void) ucontext;
    sig = (signal_info->si_value.sival_int >> 3) - SIGRTMIN;

    if (fork() == 0) {
        Block *block = NULL;
        for (uint i = 0; i < LENGTH(blocks); i += 1) {
            if (blocks[i].signal == sig) {
                block = &blocks[i];
                break;
            }
        }
        if (!block) {
            fprintf(stderr, "No block configured for signal %d\n", sig);
            exit(EXIT_SUCCESS);
        }

        // TODO: simplify this kill command
        snprintf(kill_command, sizeof (kill_command),
                 "%s && kill -%d %d", block->command, block->signal+SIGRTMIN, process_id);
        command[0] = "/bin/sh";
        command[1] = "-c";
        command[2] = kill_command;
        command[3] = NULL;

        setenv("BLOCK_BUTTON", button, 1);
        setsid();
        execvp(command[0], command);
        exit(EXIT_SUCCESS);
    }
	return;
}

FILE *popen_no_shell(char *command) {
    int pipefd[2];
    pid_t pid;

    char *c = command;
    char *argv[4] = { command, NULL };

    if (pipe(pipefd) < 0) {
        perror("pipe");
        return NULL;
    }

    while (*c) {
        if ((*c == ' ') || (*c == '\t')) {
			argv[0] = "/bin/sh";
			argv[1] = "-c";
			argv[2] = command;
			argv[3] = NULL;
            break;
        }
        c += 1;
    }

    switch (pid = fork()) {
    case 0:
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        execvp(argv[0], argv);
        fprintf(stderr, "Error executing %s: %s\n",
                        argv[0], strerror(errno));
        exit(EXIT_FAILURE);
    case -1:
        fprintf(stderr, "Error forking: %s\n", strerror(errno));
        return NULL;
    default:
        close(pipefd[1]);
        return fdopen(pipefd[0], "r");
    }
}
