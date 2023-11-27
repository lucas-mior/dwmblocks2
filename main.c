#include "dwmblocks2.h"
#include "blocks.h"
#include "util.h"

static struct pollfd pipes[LENGTH(blocks)];

static Display *display;
static Window root;

static void int_handler(int) __attribute__((noreturn));
static void parse_output(Block *);
static void signal_handler(int, siginfo_t *, void *);
static void spawn_block(Block *, int);

int main(void) {
    if (setlocale(LC_ALL, "") == NULL) {
        fprintf(stderr, "dwmblocks2: Error setting locale."
                        " Check your locale configuration.\n");
        exit(EXIT_FAILURE);
    }
    {
        struct sigaction signal_external;
        struct sigaction signal_childs;
        struct sigaction signal_int;

        signal_int.sa_handler = int_handler;
        sigemptyset(&(signal_int.sa_mask));
        sigaction(SIGINT, &signal_int, NULL);

        signal_childs.sa_handler = SIG_DFL;
        signal_childs.sa_flags = SA_NOCLDWAIT;

        sigemptyset(&(signal_childs.sa_mask));
        sigemptyset(&(signal_external.sa_mask));

        for (int i = SIGRTMIN; i <= SIGRTMAX; i += 1) {
            struct sigaction signal_this;

            sigemptyset(&(signal_this.sa_mask));
            sigaddset(&(signal_this.sa_mask), i);
            signal_int.sa_handler = SIG_IGN;
            sigaction(i, &signal_int, NULL);
        }

        for (int i = 0; i < LENGTH(blocks); i += 1) {
            struct sigaction signal_this;
            Block *block = &blocks[i];
            char *signal_string;

            if (block->signal_var_name == NULL) {
                fprintf(stderr, "Error: Signal environmental variable"
                                " must be defined for every block.\n");
                exit(EXIT_FAILURE);
            }
            if ((signal_string = getenv(block->signal_var_name)) == NULL) {
                fprintf(stderr, "Error: %s is not defined.\n",
                                block->signal_var_name);
                exit(EXIT_FAILURE);
            }

            block->signal = atoi(signal_string);
            if (block->signal <= 0) {
                fprintf(stderr, "Invalid signal for block %d."
                                "Signals must be grater than 0.\n", i);
                exit(EXIT_FAILURE);
            }
            block->signal += SIGRTMIN;
            if (block->signal >= SIGRTMAX) {
                fprintf(stderr, "Invalid signal for block."
                                "Signals must be lower than %d.\n",
                                SIGRTMAX - SIGRTMIN);
                exit(EXIT_FAILURE);
            }

            // used by dwm to send proper signal number back to dwmblocks2
            block->output[0] = (char) (block->signal - SIGRTMIN);
            block->output[1] = (char) '\0';
            block->length = 0;

            block->fd = &(pipes[i].fd);
            pipes[i].fd = -1;
            // listen only to POLLHUP to avoid partial reads
            pipes[i].events = 0;
            pipes[i].revents = 0;

            // always run the newest signal for a block, unless in
            // a critical part of handler, then sigprocmask()
            // is called on block->mask to defer newer execution
            signal_this.sa_sigaction = signal_handler;
            signal_this.sa_flags = SA_NODEFER | SA_SIGINFO;
            sigemptyset(&(block->mask));
            sigaddset(&(block->mask), block->signal);

            sigemptyset(&(signal_this.sa_mask));
            for (int j = 0; j < LENGTH(blocks); j += 1) {
                Block *other = &blocks[j];
                if (j != i)
                    sigaddset(&signal_this.sa_mask, other->signal);
            }
            sigaction(block->signal, &signal_this, NULL);
            sigaddset(&signal_external.sa_mask, block->signal);
        }

        signal_external.sa_sigaction = signal_handler;
        signal_external.sa_flags = SA_SIGINFO;
        sigaction(SIGUSR1, &signal_external, NULL);
        sigaction(SIGCHLD, &signal_childs, NULL);
    }

    if ((display = XOpenDisplay(NULL)) == NULL) {
        fprintf(stderr, "Error opening X display\n");
        exit(EXIT_FAILURE);
    }
    root = DefaultRootWindow(display);

    for (int i = 0; i < LENGTH(blocks); i += 1) {
        Block *block = &blocks[i];
        spawn_block(block, 0);
    }
    while (true) {
        static int seconds = 1;
        int ready = poll(pipes, LENGTH(blocks), 1000);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                fprintf(stderr, "Error polling: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
        }
        if (ready > 0) {
            for (int i = 0; i < LENGTH(blocks); i += 1) {
                Block *block = &blocks[i];
                if (pipes[i].revents & POLLHUP) {
                    parse_output(block);
                    continue;
                } else if (pipes[i].revents & POLLNVAL) {
                    fprintf(stderr, "Error polling: Invalid fd.\n");
                    pipes[i].fd = -1;
                } else if (pipes[i].revents & POLLERR) {
                    fprintf(stderr, "Error polling: Error condition.\n");
                    pipes[i].fd = -1;
                }
                if (block->function)
                    block->function(0, block);
            }
        } else {
            for (int i = 0; i < LENGTH(blocks); i += 1) {
                Block *block = &blocks[i];

                if (block->interval == 0)
                    continue;
                if ((seconds % block->interval) == 0)
                    spawn_block(block, 0);
            }
            seconds += 1;
        }
        {
            static char status_new[LENGTH(blocks) * (BLOCK_OUTPUT_LENGTH + 1)] = {0};
            char *pointer = status_new;

            for (int i = 0; i < LENGTH(blocks); i += 1) {
                Block *block = &blocks[i];
                char *string = block->output;
                usize size = (usize) block->length;
                if (size) {
                    memcpy(pointer, string, size);
                    pointer += size;
                }
            }
            // Apparently double '\0' means end of bar to dwm
            *pointer = '\0';

            XStoreName(display, root, status_new);
            XFlush(display);
        }
    }
}

void spawn_block(Block *block, int button) {
    int pipefd[2];
    char button_str[2] = {'0' + (char) button, '\0'};
    char *argv[3] = { block->command, button_str, NULL };

    if (block->function) {
        block->function(button, block);
        return;
    }

    sigprocmask(SIG_BLOCK, &(block->mask), NULL);

    if (*block->fd >= 0) {
        close(*block->fd);
        *block->fd = -1;
    }

    if (pipe(pipefd) < 0) {
        WRITE_ERROR("Error creating pipe: ");
        WRITE_ERROR(strerror(errno));
        WRITE_ERROR("\n");
        *block->fd = -1;
        return;
    }

    switch (fork()) {
    case 0:
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        execvp(argv[0], argv);
        WRITE_ERROR("Error executing ");
        WRITE_ERROR(block->command);
        WRITE_ERROR(": ");
        WRITE_ERROR(strerror(errno));
        WRITE_ERROR(".\n");
        _exit(EXIT_FAILURE);
    case -1:
        WRITE_ERROR("Error forking: ");
        WRITE_ERROR(strerror(errno));
        WRITE_ERROR(".\n");
        close(pipefd[0]);
        close(pipefd[1]);
        *block->fd = -1;
        break;
    default:
        close(pipefd[1]);
        *block->fd = pipefd[0];
    }

    sigprocmask(SIG_UNBLOCK, &(block->mask), NULL);
    return;
}

void parse_output(Block *block) {
    isize r;
    usize left = BLOCK_OUTPUT_LENGTH - 2;
    char *string = block->output + 1;

    sigprocmask(SIG_BLOCK, &(block->mask), NULL);

    while ((r = read(*block->fd, string, left)) > 0) {
        string += r;
        left -= (usize) r;
        if (left <= 0)
            break;
    }

    if (r < 0) {
        WRITE_ERROR("Error reading from block ");
        WRITE_ERROR(block->command);
        WRITE_ERROR(": ");
        WRITE_ERROR(strerror(errno));
        WRITE_ERROR(".\n");
    }

    close(*block->fd);
    *block->fd = -1;

    sigprocmask(SIG_UNBLOCK, &(block->mask), NULL);

    if ((r < 0) || (string == (block->output + 1))) {
        WRITE_ERROR("Read nothing from block ");
        WRITE_ERROR(block->command);
        WRITE_ERROR(".\n");

        string[0] = '\0';
        block->length = 0;
        return;
    }
    block->length = (int) (string - (block->output + 1));

    string = block->output + 1;
    string[block->length] = '\0';
    if (block->length == 0) {
        WRITE_ERROR("Read nothing from block ");
        WRITE_ERROR(block->command);
        WRITE_ERROR(".\n");

        return;
    }

    while (IS_SPACE(string[block->length - 1])) {
        string[block->length - 1] = '\0';
        block->length -= 1;
        if (block->length == 0)
            break;
    }
    if (block->length >= LONG_OUTPUT) {
        int delim = (int) strcspn(string + LONG_OUTPUT, " .:-_()[]{}");
        string[LONG_OUTPUT + delim] = '\0';
        block->length = LONG_OUTPUT + delim;
    }
    if (block->length > 0) {
        string[block->length] = ' ';
        string[block->length + 1] = '\0';
        block->length += 1;
        block->length += 1; // because of the first char with signal number
    }
    return;
}

void signal_handler(int signum, siginfo_t *signal_info, void *ucontext) {
    int button = 0;
    (void) ucontext;

    if (signum == SIGUSR1) {
        // number send by dwm
        signum = signal_info->si_value.sival_int >> 3;
        button = signal_info->si_value.sival_int & 7;
    }

    for (int i = 0; i < LENGTH(blocks); i += 1) {
        Block *block = &blocks[i];
        if (block->signal == signum)
            spawn_block(block, button);
    }
    return;
}

void int_handler(int unused) {
    (void) unused;

    for (int i = 0; i < LENGTH(blocks); i += 1) {
        Block *block = &blocks[i];
        char num[16];
        if (*block->fd >= 0) {
            WRITE_ERROR("closing block ");
            WRITE_ERROR(itoa(*block->fd, num));
            WRITE_ERROR("...\n");
            close(*block->fd);
        }
    }
    _exit(EXIT_FAILURE);
}
