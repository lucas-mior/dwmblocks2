#include "dwmblocks2.h"
#include "blocks.h"
#include "util.h"
#include <sys/param.h>
#include <assert.h>

static fd_set input_set;
static int max_fd = -1;
uint64 seconds = 0;

static Display *display;
static Window root;

static int popen_no_shell(char *, char *);
static void parse_output(Block *);
static void spawn_block(Block *, int);
static void spawn_blocks(uint64);
static void signal_handler(int, siginfo_t *, void *);
static void status_bar_update(bool);

int main(void) {
    {
        int signal_max = SIGRTMAX - SIGRTMIN;
        struct sigaction signal_external;
        struct sigaction signal_childs;
        signal_childs.sa_handler = SIG_DFL;
        signal_childs.sa_flags = SA_NOCLDWAIT;

        sigemptyset(&(signal_childs.sa_mask));
        sigemptyset(&(signal_external.sa_mask));

        for (int i = SIGRTMIN; i <= SIGRTMAX; i += 1)
            signal(i, SIG_IGN);

        for (uint i = 0; i < LENGTH(blocks); i += 1) {
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
            if (block->signal >= signal_max) {
                fprintf(stderr, "Invalid signal for block."
                                "Signals must be lower than %d.\n",
                                signal_max);
                exit(EXIT_FAILURE);
            }

            // used by dwm to send proper signal number back to dwmblocks2
            block->output[0] = (char) block->signal;
            block->output[1] = (char) '\0';

            block->pipe = -1;
            block->length = 0;

            // always run the newest signal for a block, unless in
            // a critical part of handler, then sigprocmask()
            // is called on block->mask to defer newer execution
            signal_this.sa_sigaction = signal_handler;
            signal_this.sa_flags = SA_NODEFER | SA_SIGINFO;
            sigemptyset(&(block->mask));
            sigaddset(&(block->mask), block->signal + SIGRTMIN);

            sigemptyset(&(signal_this.sa_mask));
            for (uint j = 0; j < LENGTH(blocks); j+=1) {
                Block *blockj = &blocks[j];
                sigaddset(&signal_this.sa_mask, SIGRTMIN + blockj->signal);
            }
            sigaction(SIGRTMIN + block->signal, &signal_this, NULL);
            sigaddset(&signal_external.sa_mask, SIGRTMIN + block->signal);
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

    FD_ZERO(&input_set);
    while (true) {
        struct timeval timeout;
        int ready;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        ready = select(max_fd+1, &input_set, NULL, NULL, &timeout);
        if (ready < 0) {
            switch (errno) {
                case EBADF:
                    write_error("Error in select: Bad file descriptor.\n");
                    write_error("Reseting select file descriptors"
                                " and spawning blocks again");
                    FD_ZERO(&input_set);
                    spawn_blocks(0);
                    break;
                case EINTR:
                    write_error("Select interrupted by signal.\n");
                    break;
                default:
                    write_error("Error in select: ");
                    write_error(strerror(errno));
                    write_error("\n");
                    exit(EXIT_FAILURE);
            }
            continue;
        }
        if (ready > 0) {
            for (uint i = 0; i < LENGTH(blocks); i += 1) {
                Block *block = &blocks[i];
                if (block->function) {
                    block->function(0, block);
                    continue;
                }
                if (FD_ISSET(block->pipe, &input_set)) {
                    FD_CLR(block->pipe, &input_set);
                    parse_output(block);
                } else if (block->pipe >= 0) {
                    FD_SET(block->pipe, &input_set);
                }
            }
            status_bar_update(false);
        } else {
            spawn_blocks(seconds);
            seconds += 1;
            status_bar_update(false);
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

    if (block->pipe >= 0) {
        close(block->pipe);
        FD_CLR(block->pipe, &input_set);
        block->pipe = -1;
    }

    if ((block->pipe = popen_no_shell(block->command, button_str)) < 0)
        return;

    FD_SET(block->pipe, &input_set);
    max_fd = MAX(max_fd, block->pipe);

    sigprocmask(SIG_UNBLOCK, &(block->mask), NULL);
    return;
}

void parse_output(Block *block) {
    isize r;
    usize left = BLOCK_OUTPUT_LENGTH - 1;
    char *string = block->output + 1;

    sigprocmask(SIG_BLOCK, &(block->mask), NULL);

    do {
        r = read(block->pipe, string, left);
        if (r <= 0)
            break;
        string += r;
        left -= (usize) r;
        if (left <= 0)
            break;
    } while (true);

    close(block->pipe);
    FD_CLR(block->pipe, &input_set);
    block->pipe = -1;

    sigprocmask(SIG_UNBLOCK, &(block->mask), NULL);

    if ((r < 0) || (string == (block->output + 1))) {
        string[0] = '\0';
        block->length = 0;
        return;
    }
    block->length = (uint32) (string - (block->output + 1));

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

void spawn_blocks(uint64 seconds) {
    for (uint i = 0; i < LENGTH(blocks); i += 1) {
        Block *block = &blocks[i];
        if (block->pipe >= 0)
            FD_SET(block->pipe, &input_set);
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

void status_bar_update(bool check_changed) {
    static char status_new[LENGTH(blocks) * (BLOCK_OUTPUT_LENGTH+1)] = {0};
    char *pointer = status_new;
    (void) check_changed;

    for (uint i = 0; i < LENGTH(blocks); i += 1) {
        Block *block = &blocks[i];
        char *string = block->output;
        usize size = block->length;
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
    signum -= SIGRTMIN;
    for (uint i = 0; i < LENGTH(blocks); i += 1) {
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
        exit(EXIT_FAILURE);
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
