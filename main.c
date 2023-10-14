#include "dwmblocks2.h"
#include "blocks.h"
#include "util.h"

static struct pollfd pipes[LENGTH(blocks)];

static Display *display;
static Window root;

static int popen_no_shell(char *, char *);
static void parse_output(Block *);
static void spawn_block(Block *, int);
static void spawn_blocks(int);
static void signal_handler(int, siginfo_t *, void *);
static void status_bar_update(void);

int main(void) {
    int seconds = 0;
    {
        struct sigaction signal_external;
        struct sigaction signal_childs;
        signal_childs.sa_handler = SIG_DFL;
        signal_childs.sa_flags = SA_NOCLDWAIT;

        sigemptyset(&(signal_childs.sa_mask));
        sigemptyset(&(signal_external.sa_mask));

        for (int i = SIGRTMIN; i <= SIGRTMAX; i += 1)
            signal(i, SIG_IGN);

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

            block->pipe = &(pipes[i].fd);
            pipes[i].fd = -1;
            pipes[i].events = 0; // listen only to POLLHUP to avoid partial reads
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

    while (true) {
        int ready = poll(pipes, LENGTH(blocks), 1000);
        if (ready < 0) {
            fprintf(stderr, "poll() error: %s\n", strerror(errno));
            continue;
        }
        if (ready > 0) {
            fprintf(stderr, "%d block ready!\n", ready);
            for (int i = 0; i < LENGTH(blocks); i += 1) {
                Block *block = &blocks[i];
                if (pipes[i].revents & POLLHUP) {
                    parse_output(block);
                    continue;
                } else if (pipes[i].revents & POLLNVAL) {
                    pipes[i].fd = -1;
                } else if (pipes[i].revents & POLLERR) {
                    write_error("poll returned POLLERR.\n");
                }
                if (block->function)
                    block->function(0, block);
            }
            status_bar_update();
        } else {
            write_error("poll timeout!!!\n");
            spawn_blocks(seconds);
            seconds += 1;
            status_bar_update();
        }
    }
}

void spawn_block(Block *block, int button) {
    char button_str[2] = {'0' + (char) button, '\0'};

    if (block->function) {
        block->function(button, block);
        return;
    }

    sigprocmask(SIG_BLOCK, &(block->mask), NULL);

    if (*block->pipe >= 0) {
        close(*block->pipe);
        *block->pipe = -1;
    }

    *block->pipe = popen_no_shell(block->command, button_str);
    if (*block->pipe < 0)
        return;

    sigprocmask(SIG_UNBLOCK, &(block->mask), NULL);
    return;
}

void parse_output(Block *block) {
    isize r;
    usize left = BLOCK_OUTPUT_LENGTH - 1;
    char *string = block->output + 1;

    sigprocmask(SIG_BLOCK, &(block->mask), NULL);

    while ((r = read(*block->pipe, string, left)) > 0) {
        string += r;
        left -= (usize) r;
        if (left <= 0)
            break;
    }

    close(*block->pipe);
    *block->pipe = -1;

    sigprocmask(SIG_UNBLOCK, &(block->mask), NULL);

    if ((r < 0) || (string == (block->output + 1))) {
        string[0] = '\0';
        block->length = 0;
        return;
    }
    block->length = (int) (string - (block->output + 1));

    string = block->output + 1;
    string[block->length] = '\0';
    if (block->length == 0)
        return;

    while (IS_SPACE(string[block->length - 1])) {
        string[block->length - 1] = '\0';
        block->length -= 1;
        if (block->length == 0)
            break;
    }
    if (block->length > 0) {
        string[block->length] = ' ';
        string[block->length + 1] = '\0';
        block->length += 1;
        block->length += 1; // because of the first char with signal number
    }
    return;
}

void spawn_blocks(int seconds) {
    for (int i = 0; i < LENGTH(blocks); i += 1) {
        Block *block = &blocks[i];
        if (seconds == 0) {
            spawn_block(block, 0);
            continue;
        }
        if (block->interval == 0)
            continue;
        if ((seconds % block->interval) == 0)
            spawn_block(block, 0);
    }
    return;
}

void status_bar_update(void) {
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

int popen_no_shell(char *command, char *button) {
    int pipefd[2];
    char *argv[3] = { command, button, NULL };

    if (pipe(pipefd) < 0) {
        write_error("Error creating pipe: ");
        write_error(strerror(errno));
        write_error("\n");
        return -1;
    }

    switch (fork()) {
    case 0:
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        execvp(argv[0], argv);
        write_error("Error executing");
        write_error(command);
        write_error(": ");
        write_error(strerror(errno));
        write_error(".\n");
        _exit(EXIT_FAILURE);
    case -1:
        write_error("Error forking: ");
        write_error(strerror(errno));
        write_error(".\n");
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    default:
        close(pipefd[1]);
        return pipefd[0];
    }
}
