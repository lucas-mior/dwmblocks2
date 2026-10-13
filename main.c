// SPDX-License-Identifier: AGPL
// Copyright (c) 2026 Lucas Mior

#define CBASE_IMPLEMENT
#include "cbase.h"

#include "dwmblocks2.h"
#include "blocks.h"

#define NANOSECONDS_PER_SECOND 1000000000ll
#define NANOSECONDS_PER_MILLISECOND 1000000ll
#define STATUS_BUFFER_SIZE (LENGTH(blocks)*MAX_BLOCK_OUTPUT_LENGTH + 2)

static struct pollfd pipes[LENGTH(blocks) + 1];

#define SIGNAL_PIPE_INDEX LENGTH(blocks)

static int signal_pipe[2] = {-1, -1};
static sigset_t handled_signal_mask;
static volatile sig_atomic_t block_spawn_requests[LENGTH(blocks)];
static int64 next_update_ns[LENGTH(blocks)];

static Display *display;
static Window root;

static noreturn void int_handler(int);
static void drain_signal_pipe(void);
static void fill_handled_signal_mask(sigset_t *);
static void parse_output(Block *);
static void publish_status(void);
static void signal_handler(int, siginfo_t *, void *);
static void spawn_block(Block *, int);

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
        sigfillset(&(signal_int.sa_mask));
        sigaction(SIGINT, &signal_int, NULL);

        signal_childs.sa_handler = SIG_DFL;
        signal_childs.sa_flags = SA_NOCLDWAIT;

        sigemptyset(&(signal_childs.sa_mask));
        sigemptyset(&(signal_external.sa_mask));

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

#if defined(SIGRTMIN)
        for (int i = SIGRTMIN; i <= SIGRTMAX; i += 1) {
            struct sigaction signal_this = {0};

            signal_this.sa_handler = SIG_IGN;
            sigemptyset(&(signal_this.sa_mask));
            sigaction(i, &signal_this, NULL);
        }
#endif

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
#if defined(SIGRTMIN)
            block->signal += SIGRTMIN;
            if (block->signal > SIGRTMAX) {
                error("Invalid signal for block."
                      " Signals must be lower than %d.\n",
                      SIGRTMAX - SIGRTMIN);
                exit(EXIT_FAILURE);
            }

            // used by dwm to send proper signal number back to dwmblocks2
            block->output[0] = (char)(block->signal - SIGRTMIN + 1);
#else
            block->output[0] = (char)block->signal + 1;
#endif
            block->output[1] = (char)'\0';
            block->length = 0;
            block->cmd_len = 0;
            if (block->command != NULL) {
                block->cmd_len = strlen32(block->command);
            }

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

    {
        Atom pid_atom = XInternAtom(display, STATUS_PID_PROPERTY, False);
        Atom owner_atom = XInternAtom(display, STATUS_OWNER_SELECTION, False);
        Window owner = XCreateSimpleWindow(display, root, 0, 0, 1, 1, 0, 0, 0);
        ulong identity[2] = {(ulong)getpid(), (ulong)owner};

        // The root property identifies the process and its X11 lifetime.
        // X11 automatically clears selection ownership when we disconnect.
        XSetSelectionOwner(display, owner_atom, owner, CurrentTime);
        XChangeProperty(display, root, pid_atom, XA_CARDINAL, 32,
                        PropModeReplace, (uchar *)identity, LENGTH(identity));
        XFlush(display);
    }

    {
        int64 now = time_monotonic_now();

        for (int32 i = 0; i < LENGTH(blocks); i += 1) {
            Block *block = &blocks[i];

            if (block->interval < 0) {
                error("Invalid negative interval for block %d.\n", i);
                exit(EXIT_FAILURE);
            }
            if (block->interval > 0) {
                next_update_ns[i] =
                    now + (int64)block->interval*NANOSECONDS_PER_SECOND;
            }
            spawn_block(block, 0);
        }
    }
    publish_status();

    while (true) {
        int32 nready;
        int64 now = time_monotonic_now();
        int64 wait_ns = INT64_MAX;
        int32 timeout_ms = -1;

        for (int32 i = 0; i < LENGTH(blocks); i += 1) {
            if (blocks[i].interval <= 0) {
                continue;
            }
            if (next_update_ns[i] <= now) {
                wait_ns = 0;
                break;
            }
            if ((next_update_ns[i] - now) < wait_ns) {
                wait_ns = next_update_ns[i] - now;
            }
        }
        if (wait_ns == 0) {
            timeout_ms = 0;
        } else if (wait_ns != INT64_MAX) {
            if (wait_ns >= (int64)INT32_MAX*NANOSECONDS_PER_MILLISECOND) {
                timeout_ms = INT32_MAX;
            } else {
                // Round up so poll cannot spin before the next deadline.
                timeout_ms =
                    (int32)((wait_ns + NANOSECONDS_PER_MILLISECOND - 1)
                            / NANOSECONDS_PER_MILLISECOND);
            }
        }

        nready = poll(pipes, LENGTH(pipes), timeout_ms);
        if (nready < 0) {
            if (errno != EINTR) {
                error("Error polling: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            drain_signal_pipe();
        } else if (nready > 0) {
            for (int32 i = 0; i < LENGTH(blocks); i += 1) {
                Block *block = &blocks[i];
                int32 events = pipes[i].revents;

                if (events & POLLHUP) {
                    parse_output(block);
                } else if (events & (POLLERR | POLLNVAL)) {
                    error("Error polling block %d: events %d.\n", i, events);
                    if (*block->fd >= 0) {
                        XCLOSE(block->fd);
                    }
                }
            }

            if (pipes[SIGNAL_PIPE_INDEX].revents & POLLIN) {
                drain_signal_pipe();
            } else if (pipes[SIGNAL_PIPE_INDEX].revents
                       & (POLLERR | POLLNVAL | POLLHUP)) {
                error("Error polling signal pipe.\n");
                exit(EXIT_FAILURE);
            }
        }

        // Deadlines are evaluated after every wake, including signals and I/O.
        // The newest explicit signal takes priority over a simultaneous tick.
        {
            int32 buttons[LENGTH(blocks)];
            sigset_t old_mask;

            now = time_monotonic_now();
            for (int32 i = 0; i < LENGTH(blocks); i += 1) {
                Block *block = &blocks[i];

                buttons[i] = -1;
                if ((block->interval > 0) && (now >= next_update_ns[i])) {
                    int64 interval_ns =
                        (int64)block->interval*NANOSECONDS_PER_SECOND;
                    int64 elapsed = now - next_update_ns[i];

                    // Skip missed periods instead of accumulating a backlog.
                    next_update_ns[i] += (elapsed/interval_ns + 1)*interval_ns;
                    buttons[i] = 0;
                }
            }

            if (sigprocmask(SIG_BLOCK, &handled_signal_mask, &old_mask) < 0) {
                error("Error blocking signals: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            for (int32 i = 0; i < LENGTH(blocks); i += 1) {
                sig_atomic_t request = block_spawn_requests[i];

                if (request != 0) {
                    buttons[i] = (int32)request - 1;
                    block_spawn_requests[i] = 0;
                }
            }
            if (sigprocmask(SIG_SETMASK, &old_mask, NULL) < 0) {
                error("Error restoring signal mask: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }

            for (int32 i = 0; i < LENGTH(blocks); i += 1) {
                if (buttons[i] >= 0) {
                    spawn_block(&blocks[i], buttons[i]);
                }
            }
        }
        publish_status();
    }
}


void
drain_signal_pipe(void) {
    char buffer[32];
    int64 r;

    while ((r = read64(signal_pipe[0], buffer, SIZEOF(buffer))) > 0) {
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
publish_status(void) {
    static char previous[STATUS_BUFFER_SIZE] = {0};
    static int64 previous_len;
    static bool published;
    char status_new[STATUS_BUFFER_SIZE] = {0};
    char *pointer = status_new;
    int64 length;

    for (int32 i = 0; i < LENGTH(blocks); i += 1) {
        Block *block = &blocks[i];

        if (block->length > 1) {
            memcpy64(pointer, block->output, block->length);
            pointer += block->length;
        }
        if (i == (LENGTH(blocks) / 2)) {
            *pointer = DWM_BAR_SEPARATOR;
            pointer += 1;
        }
    }
    length = pointer - status_new;

    if (DEBUGGING) {
        static int64 start_ns;
        static bool dumped;

        if (start_ns == 0) {
            start_ns = time_monotonic_now();
        }
        if (!dumped && ((time_monotonic_now() - start_ns)
                        >= 10*NANOSECONDS_PER_SECOND)) {
            char *name = "dwmblocks2.txt";
            FILE *file = fopen(name, "w");

            if (file == NULL) {
                error("Error opening %s: %s\n", name, strerror(errno));
                exit(EXIT_FAILURE);
            }
            fwrite64(status_new, SIZEOF(*status_new), SIZEOF(status_new), file);
            fclose(file);
            dumped = true;
        }
    }

    if (!published || (previous_len != length)
        || (memcmp64(previous, status_new, length) != 0)) {
        XStoreName(display, root, status_new);
        XFlush(display);
        memcpy64(previous, status_new, length + 1);
        previous_len = length;
        published = true;
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

        error_async_safe(STRLIT("Error creating pipe: "));
        error_async_safe(error_message, strlen32(error_message));
        error_async_safe(STRLIT("\n"));

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

        error_async_safe(STRLIT("Error executing "));
        error_async_safe(block->command, block->cmd_len);
        error_async_safe(STRLIT(": "));
        error_async_safe(error_message, strlen32(error_message));
        error_async_safe(STRLIT(".\n"));

        _exit(EXIT_FAILURE);
    case -1:
        strerror_r(errno, error_message, sizeof(error_message));

        error_async_safe(STRLIT("Error forking: "));
        error_async_safe(error_message, strlen32(error_message));
        error_async_safe(STRLIT(".\n"));

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
    int64 space = SIZEOF(block->output) - 3;
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
        error_async_safe(STRLIT("Error reading from block "));
        error_async_safe(block->command, block->cmd_len);
        error_async_safe(STRLIT(": "));
        error_async_safe(error_message, strlen32(error_message));
        error_async_safe(STRLIT(".\n"));
    }

    XCLOSE(block->fd);

    sigprocmask(SIG_UNBLOCK, &(block->mask), NULL);

    if ((r < 0) || (string == (block->output + 1))) {
        error_async_safe(STRLIT("Read nothing from block "));
        error_async_safe(block->command, block->cmd_len);
        error_async_safe(STRLIT(".\n"));

        string[0] = '\0';
        block->length = 0;
        return;
    }
    block->length = (int)(string - (block->output + 1));

    string = block->output + 1;
    string[block->length] = '\0';
    if (block->length == 0) {
        error_async_safe(STRLIT("Read nothing from block "));
        error_async_safe(block->command, block->cmd_len);
        error_async_safe(STRLIT(".\n"));

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
        error_async_safe(STRLIT("Block length is invalid.\n"));
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

    for (int i = 0; i < LENGTH(blocks); i += 1) {
        Block *block = &blocks[i];
        if (block->signal == signum) {
            block_spawn_requests[i] = button + 1;
        }
    }
    if (signal_pipe[1] >= 0) {
        write64(signal_pipe[1], &byte, SIZEOF(byte));
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
        int32 num_len;

        if (*block->fd >= 0) {
            num_len = ITOA(num, *block->fd);

            error_async_safe(STRLIT("closing block "));
            error_async_safe(num, num_len);
            error_async_safe(STRLIT("...\n"));

            if (XCLOSE(block->fd) < 0) {
                strerror_r(errno, error_message, sizeof(error_message));
                error_async_safe(STRLIT("Error closing: "));
                error_async_safe(error_message, strlen32(error_message));
                error_async_safe(STRLIT(".\n"));
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
