// SPDX-License-Identifier: AGPL
// Copyright (c) 2026 Lucas Mior

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <locale.h>
#include <fcntl.h>

#include "dwmblocks2.h"
#include "blocks.h"

#define CBASE_IMPLEMENT
#include "cbase.h"


#define CLOCK CLOCK_REALTIME

static struct pollfd pipes[LENGTH(blocks) + 1];

#define SIGNAL_PIPE_INDEX LENGTH(blocks)

static int signal_pipe[2] = {-1, -1};
static sigset_t handled_signal_mask;
static volatile sig_atomic_t block_spawn_requests[LENGTH(blocks)];

static Display *display;
static Window root;

#define TIMEOUT_INTERRUPTED 350
#define TIMEOUT_NORMAL 1000

static void int_handler(int) __attribute__((noreturn));
static void drain_signal_pipe(void);
static void fill_handled_signal_mask(sigset_t *);
static void parse_output(Block *);
static void signal_handler(int, siginfo_t *, void *);
static void setup_signal_pipe(void);
static void spawn_block(Block *, int);
static void spawn_queued_blocks(void);
static volatile sig_atomic_t timeout = TIMEOUT_NORMAL;

int
main(int argc, char **argv) {
    program = argv[0];
    (void)argc;
    if (setlocale(LC_ALL, "") == NULL) {
        error("Error setting locale. Check your locale configuration.\n");
        exit(EXIT_FAILURE);
    }
    {
        struct sigaction signal_external = {0};
        struct sigaction signal_childs = {0};
        struct sigaction signal_int = {0};

        signal_int.sa_handler = int_handler;
        sigemptyset(&(signal_int.sa_mask));
        sigaction(SIGINT, &signal_int, NULL);

        signal_childs.sa_handler = SIG_DFL;
        signal_childs.sa_flags = SA_NOCLDWAIT;

        sigemptyset(&(signal_childs.sa_mask));
        sigemptyset(&(signal_external.sa_mask));

        setup_signal_pipe();

        for (int i = SIGRTMIN; i <= SIGRTMAX; i += 1) {
            struct sigaction signal_this = {0};

            signal_this.sa_handler = SIG_IGN;
            sigemptyset(&(signal_this.sa_mask));
            sigaction(i, &signal_this, NULL);
        }

        for (int i = 0; i < LENGTH(blocks); i += 1) {
            Block *block = &blocks[i];
            char *signal_string;

            if (block->signal_var_name == NULL) {
                error("Error: signal environmental variable"
                      " must be defined for every block.\n");
                exit(EXIT_FAILURE);
            }
            if ((signal_string = getenv(block->signal_var_name)) == NULL) {
                error("Error: %s is not defined.\n", block->signal_var_name);
                exit(EXIT_FAILURE);
            }

            if (!util_is_integer(signal_string)) {
                error("Error: %s is not an integer.\n", signal_string);
                exit(EXIT_FAILURE);
            }
            block->signal = atoi(signal_string);
            if (block->signal <= 0) {
                error("Invalid signal for block %d."
                      " Signals must be grater than 0.\n",
                      i);
                exit(EXIT_FAILURE);
            }
            block->signal += SIGRTMIN;
            if (block->signal > SIGRTMAX) {
                error("Invalid signal for block."
                      " Signals must be lower than %d.\n",
                      SIGRTMAX - SIGRTMIN);
                exit(EXIT_FAILURE);
            }

            // used by dwm to send proper signal number back to dwmblocks2
            block->output[0] = (char)(block->signal - SIGRTMIN + 1);
            block->output[1] = (char)'\0';
            block->length = 0;

            block->fd = &(pipes[i].fd);
            pipes[i].fd = -1;
            // listen only to POLLHUP to avoid partial reads
            pipes[i].events = 0;
            pipes[i].revents = 0;

            // always run the newest signal for a block, unless in
            // a critical part of handler, then sigprocmask()
            // is called on block->mask to defer newer execution
            sigemptyset(&(block->mask));
            sigaddset(&(block->mask), block->signal);
            sigaddset(&(block->mask), SIGUSR1);
        }

        fill_handled_signal_mask(&handled_signal_mask);
        fill_handled_signal_mask(&(signal_external.sa_mask));

        for (int i = 0; i < LENGTH(blocks); i += 1) {
            Block *block = &blocks[i];
            struct sigaction signal_this;

            signal_this.sa_sigaction = signal_handler;
            signal_this.sa_flags = SA_SIGINFO;
            fill_handled_signal_mask(&(signal_this.sa_mask));
            sigaction(block->signal, &signal_this, NULL);
        }

        signal_external.sa_sigaction = signal_handler;
        signal_external.sa_flags = SA_SIGINFO;
        sigaction(SIGUSR1, &signal_external, NULL);
        sigaction(SIGCHLD, &signal_childs, NULL);
    }

    if ((display = XOpenDisplay(NULL)) == NULL) {
        error("Error opening X display\n");
        exit(EXIT_FAILURE);
    }
    root = DefaultRootWindow(display);

    for (int i = 0; i < LENGTH(blocks); i += 1) {
        Block *block = &blocks[i];
        spawn_block(block, 0);
    }
    while (true) {
        static int seconds = 1;
        int nready;
        struct timespec t0;
        struct timespec t1;

        if (clock_gettime(CLOCK, &t0) < 0) {
            error("Error getting clock: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        nready = poll(pipes, LENGTH(pipes), timeout);
        if (nready < 0) {
            if (errno == EINTR) {
                drain_signal_pipe();
                spawn_queued_blocks();
            } else {
                error("Error polling: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
        }
        if (nready > 0) {
            bool block_events = false;

            for (int i = 0; i < LENGTH(blocks); i += 1) {
                if (pipes[i].revents != 0) {
                    block_events = true;
                    break;
                }
            }
            if (block_events && (timeout == TIMEOUT_NORMAL)) {
                struct timespec complete;

                if (clock_gettime(CLOCK, &t1) < 0) {
                    error("Error getting clock: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }

                complete.tv_sec = t1.tv_sec - t0.tv_sec;
                complete.tv_nsec = t1.tv_nsec - t0.tv_nsec;
                if (complete.tv_nsec < 0) {
                    complete.tv_nsec += 999999999;
                    complete.tv_sec -= 1;
                }

                if (complete.tv_sec < 1) {
                    complete.tv_sec = 0;
                    complete.tv_nsec = 999999999 - complete.tv_nsec;
                    if (nanosleep(&complete, NULL) < 0) {
                        continue;
                    }
                }
                seconds += 1;
                timeout = TIMEOUT_NORMAL;
            }
            if (block_events) {
                for (int i = 0; i < LENGTH(blocks); i += 1) {
                    Block *block = &blocks[i];
                    if (pipes[i].revents & POLLHUP) {
                        parse_output(block);
                        continue;
                    } else if (pipes[i].revents & POLLNVAL) {
                        error("Error polling: Invalid fd.\n");
                        pipes[i].fd = -1;
                    } else if (pipes[i].revents & POLLERR) {
                        error("Error polling: Error condition.\n");
                        pipes[i].fd = -1;
                    }
                    if (block->function) {
                        block->function(0, block);
                    }
                }
            }
            if (pipes[SIGNAL_PIPE_INDEX].revents & POLLIN) {
                drain_signal_pipe();
            } else if (pipes[SIGNAL_PIPE_INDEX].revents & POLLNVAL) {
                error("Error polling: Invalid signal pipe fd.\n");
                exit(EXIT_FAILURE);
            } else if (pipes[SIGNAL_PIPE_INDEX].revents & POLLERR) {
                error("Error polling: Signal pipe error condition.\n");
                exit(EXIT_FAILURE);
            }
            spawn_queued_blocks();
        } else if (nready == 0) {
            for (int i = 0; i < LENGTH(blocks); i += 1) {
                Block *block = &blocks[i];

                if (block->interval == 0) {
                    continue;
                }
                if ((seconds % block->interval) == 0) {
                    spawn_block(block, 0);
                }
            }
            seconds += 1;
            timeout = TIMEOUT_NORMAL;
            spawn_queued_blocks();
        }
        {
            char status_new[LENGTH(blocks)*MAX_BLOCK_OUTPUT_LENGTH] = {0};
            char *pointer = status_new;

            for (int i = 0; i < LENGTH(blocks); i += 1) {
                Block *block = &blocks[i];
                char *string = block->output;
                int64 size = block->length;
                if (size > 1) {
                    memcpy64(pointer, string, size);
                    pointer += size;
                }
                if (i == (LENGTH(blocks) / 2)) {
                    *pointer = DWM_BAR_SEPARATOR;
                    pointer += 1;
                }
            }

            if (DEBUGGING) {
                if (seconds == 10) {
                    char *name = "dwmblocks2.txt";
                    FILE *file;
                    if (!(file = fopen(name, "w"))) {
                        error("Error opening %s: %s\n", name, strerror(errno));
                        exit(EXIT_FAILURE);
                    }
                    fwrite64(status_new, sizeof(*status_new),
                             sizeof(status_new), file);
                    fclose(file);
                }
            }

            XStoreName(display, root, status_new);
            XFlush(display);
        }
    }
}


void
drain_signal_pipe(void) {
    char buffer[128];
    int64 r;

    while ((r = read64(signal_pipe[0], buffer, sizeof(buffer))) > 0) {
        continue;
    }
    if ((r < 0) && (errno != EAGAIN) && (errno != EWOULDBLOCK)) {
        error("Error reading signal pipe: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    return;
}

void
fill_handled_signal_mask(sigset_t *mask) {
    sigemptyset(mask);
    sigaddset(mask, SIGUSR1);
    for (int i = 0; i < LENGTH(blocks); i += 1) {
        sigaddset(mask, blocks[i].signal);
    }
    return;
}

void
setup_signal_pipe(void) {
    if (pipe(signal_pipe) < 0) {
        error("Error creating signal pipe: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < LENGTH(signal_pipe); i += 1) {
        int flags = fcntl(signal_pipe[i], F_GETFL);

        if (flags < 0) {
            error("Error getting signal pipe flags: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        if (fcntl(signal_pipe[i], F_SETFL, flags | O_NONBLOCK) < 0) {
            error("Error setting signal pipe flags: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    pipes[SIGNAL_PIPE_INDEX].fd = signal_pipe[0];
    pipes[SIGNAL_PIPE_INDEX].events = POLLIN;
    pipes[SIGNAL_PIPE_INDEX].revents = 0;
    return;
}

void
spawn_queued_blocks(void) {
    int buttons[LENGTH(blocks)];
    sigset_t old_mask;

    for (int i = 0; i < LENGTH(blocks); i += 1) {
        buttons[i] = -1;
    }

    if (sigprocmask(SIG_BLOCK, &handled_signal_mask, &old_mask) < 0) {
        error("Error blocking signals: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < LENGTH(blocks); i += 1) {
        sig_atomic_t request = block_spawn_requests[i];

        if (request != 0) {
            buttons[i] = (int)request - 1;
            block_spawn_requests[i] = 0;
        }
    }
    if (sigprocmask(SIG_SETMASK, &old_mask, NULL) < 0) {
        error("Error restoring signal mask: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < LENGTH(blocks); i += 1) {
        if (buttons[i] >= 0) {
            spawn_block(&blocks[i], buttons[i]);
        }
    }
    return;
}

void
spawn_block(Block *block, int button) {
    int pipefd[2];
    char button_str[2] = {'0' + (char)button, '\0'};
    char *argv[3] = {block->command, button_str, NULL};
    char error_message[1024];

    if (block->function) {
        block->function(button, block);
        return;
    }

    sigprocmask(SIG_BLOCK, &(block->mask), NULL);

    if (*block->fd >= 0) {
        XCLOSE(block->fd, block->command);
    }

    if (pipe(pipefd) < 0) {
        strerror_r(errno, error_message, sizeof(error_message));

        error_async_safe("Error creating pipe: ");
        error_async_safe(error_message);
        error_async_safe("\n");

        *block->fd = -1;

        sigprocmask(SIG_UNBLOCK, &(block->mask), NULL);
        return;
    }

    switch (fork()) {
    case 0:
        XCLOSE(&pipefd[0]);
        xdup2(pipefd[1], STDOUT_FILENO);
        XCLOSE(&pipefd[1]);
        sigprocmask(SIG_UNBLOCK, &(block->mask), NULL);
        execvp(argv[0], argv);
        strerror_r(errno, error_message, sizeof(error_message));

        error_async_safe("Error executing ");
        error_async_safe(block->command);
        error_async_safe(": ");
        error_async_safe(error_message);
        error_async_safe(".\n");

        _exit(EXIT_FAILURE);
    case -1:
        strerror_r(errno, error_message, sizeof(error_message));

        error_async_safe("Error forking: ");
        error_async_safe(error_message);
        error_async_safe(".\n");

        XCLOSE(&pipefd[0]);
        XCLOSE(&pipefd[1]);
        *block->fd = -1;
        break;
    default:
        XCLOSE(&pipefd[1]);
        *block->fd = pipefd[0];
        break;
    }

    sigprocmask(SIG_UNBLOCK, &(block->mask), NULL);
    return;
}

void
parse_output(Block *block) {
    int64 r;
    int64 space = sizeof(block->output) - 3;
    char *string = block->output + 1;
    char error_message[1024];

    sigprocmask(SIG_BLOCK, &(block->mask), NULL);

    while ((r = read64(*block->fd, string, space)) > 0) {
        string += r;
        space -= r;
        if (space <= 0) {
            break;
        }
    }

    if (r < 0) {
        strerror_r(errno, error_message, sizeof(error_message));
        error_async_safe("Error reading from block ");
        error_async_safe(block->command);
        error_async_safe(": ");
        error_async_safe(error_message);
        error_async_safe(".\n");
    }

    XCLOSE(block->fd);

    sigprocmask(SIG_UNBLOCK, &(block->mask), NULL);

    if ((r < 0) || (string == (block->output + 1))) {
        error_async_safe("Read nothing from block ");
        error_async_safe(block->command);
        error_async_safe(".\n");

        string[0] = '\0';
        block->length = 0;
        return;
    }
    block->length = (int)(string - (block->output + 1));

    string = block->output + 1;
    string[block->length] = '\0';
    if (block->length == 0) {
        error_async_safe("Read nothing from block ");
        error_async_safe(block->command);
        error_async_safe(".\n");

        return;
    }

    while (IS_SPACE(string[block->length - 1])) {
        string[block->length - 1] = '\0';
        block->length -= 1;
        if (block->length == 0) {
            return;
        }
    }
    if (block->length > 0) {
        string[block->length] = ' ';
        string[block->length + 1] = '\0';
        block->length += 1;
        block->length += 1;  // because of the first char with signal number
    }
    for (int32 i = 0; i < (block->length - 1); i += 1) {
        while ((uchar)string[i] < ' ') {
            block->length -= 1;
            if (block->length <= i) {
                goto final;
            }

            memmove64(&string[i], &string[i + 1],
                      (block->length - i)*SIZEOF(*string));
        }
    }
final:
    if (block->length <= 0) {
        error_async_safe("Block length is less than or equal to zero.\n");
        exit(EXIT_FAILURE);
    }
    return;
}

void
signal_handler(int signum, siginfo_t *signal_info, void *ucontext) {
    int saved_errno = errno;
    int button = 0;
    char byte = 1;
    (void)ucontext;

    if (signum == SIGUSR1) {
        // dwm sends SIGRTMIN + the status byte in the high bits.  The
        // status byte is one more than the block's relative realtime signal.
        signum = (signal_info->si_value.sival_int >> 3) - 1;
        button = signal_info->si_value.sival_int & 7;
    }

    timeout = TIMEOUT_INTERRUPTED;

    for (int i = 0; i < LENGTH(blocks); i += 1) {
        Block *block = &blocks[i];
        if (block->signal == signum) {
            block_spawn_requests[i] = button + 1;
            break;
        }
    }
    if (signal_pipe[1] >= 0) {
        (void)write(signal_pipe[1], &byte, sizeof(byte));
    }
    errno = saved_errno;
    return;
}

void
int_handler(int unused) {
    (void)unused;

    for (int i = 0; i < LENGTH(blocks); i += 1) {
        Block *block = &blocks[i];
        char error_message[1024];
        char num[32];

        if (*block->fd >= 0) {
            ITOA(num, *block->fd);

            error_async_safe("closing block ");
            error_async_safe(num);
            error_async_safe("...\n");

            if (XCLOSE(block->fd) < 0) {
                strerror_r(errno, error_message, sizeof(error_message));
                error_async_safe("Error closing: ");
                error_async_safe(error_message);
                error_async_safe(".\n");
            }
        }
    }

    if (signal_pipe[0] >= 0) {
        XCLOSE(&signal_pipe[0]);
    }
    if (signal_pipe[1] >= 0) {
        XCLOSE(&signal_pipe[1]);
    }

    _exit(EXIT_FAILURE);
}
