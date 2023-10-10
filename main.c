#include "dwmblocks2.h"
#include "blocks.h"

static Display *display;
static Window root;
static Output status_bar[LENGTH(blocks)] = {0};
static const char delim = ' ';

static int popen_no_shell(char *);
static void button_block(char *, Block *);
static void button_handler(int, siginfo_t *, void *);
static void get_block_output(const Block *, Output *);
static void get_block_outputs(int64);
static void signal_handler(int);
static void status_bar_update(bool);

int main(void) {
    {
        int sig_max = SIGRTMAX - SIGRTMIN;
        struct sigaction signal_external;
        struct sigaction signal_childs;
        signal_childs.sa_handler = SIG_DFL;
        signal_childs.sa_flags = SA_NOCLDWAIT;

        for (int i = SIGRTMIN; i <= SIGRTMAX; i += 1)
            signal(i, SIG_IGN);

        for (uint i = 0; i < LENGTH(blocks); i += 1) {
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
            if (blocks->signal <= 0) {
                fprintf(stderr, "Invalid signal for block %d."
                                "Signals must be grater than 0.\n", i);
                exit(EXIT_FAILURE);
            }
            if (blocks->signal >= sig_max) {
                fprintf(stderr, "Invalid signal for block."
                                "Signals must be lower than %d.\n",
                                sig_max);
                exit(EXIT_FAILURE);
            }

            signal(SIGRTMIN + block->signal, signal_handler);
            sigaddset(&signal_external.sa_mask, SIGRTMIN + block->signal);
            // used by dwm to send proper signal number back to dwmblocks2
            status_bar[i].string[0] = (char) block->signal;
        }

        signal_external.sa_sigaction = button_handler;
        signal_external.sa_flags = SA_SIGINFO | SA_NODEFER;
        sigaction(SIGUSR1, &signal_external, NULL);
        sigaction(SIGCHLD, &signal_childs, NULL);
    }

    if ((display = XOpenDisplay(NULL)) == NULL) {
        fprintf(stderr, "Error opening X display\n");
        exit(EXIT_FAILURE);
    }

    root = DefaultRootWindow(display);
    {
        struct timespec sleep_time;
        struct timespec to_sleep;
        int64 seconds = -1;
        sleep_time.tv_sec = 1;
        sleep_time.tv_nsec = 0;

        while (true) {
            to_sleep = sleep_time;
            get_block_outputs(seconds);
            status_bar_update(false);

            while (nanosleep(&to_sleep, &to_sleep) < 0);
            seconds += sleep_time.tv_sec;
        }
    }
}

void get_block_output(const Block *block, Output *out) {
    int command_pipe;
    char *string = out->string + 1;
    isize r;
    usize left = BLOCK_OUTPUT_LENGTH - 1;

    if (block->function) {
        block->function(0, out);
        return;
    }
    if ((command_pipe = popen_no_shell(block->command)) < 0) {
        char *msg = "Failed to run ";
        char *error = strerror(errno);
        write(STDERR_FILENO, msg, strlen(msg));
        write(STDERR_FILENO, block->command, strlen(block->command));
        write(STDERR_FILENO, ": ", 2);
        write(STDERR_FILENO, error, strlen(error));
        write(STDERR_FILENO, "\n", 1);

        string[0] = '\0';
        return;
    }

    while ((r = read(command_pipe, string, left)) > 0) {
        string += r;
        left -= r;
        if (left <= 0)
            break;
    }
    close(command_pipe);
    
    if ((r < 0) || (string == (out->string + 1))) {
        string[0] = '\0';
        out->length = 0;
        return;
    }

    out->length = (uint32) (string - (out->string + 1));

    string = out->string + 1;
    string[out->length] = '\0';
    if (out->length == 0)
        return;

    while (IS_SPACE(string[out->length - 1])) {
        string[out->length - 1] = '\0';
        out->length -= 1;
        if (out->length == 0)
            break;
    }
    if (out->length > 0) {
        string[out->length] = delim;
        string[out->length + 1] = '\0';
        out->length += 1;
        out->length += 1; // because of the first char containing signal number
    }
    return;
}

void get_block_outputs(int64 seconds) {
    for (uint i = 0; i < LENGTH(blocks); i += 1) {
        Block *block = &blocks[i];
        if (seconds < 0) {
            get_block_output(block, &status_bar[i]);
            continue;
        }
        if (block->interval == 0)
            continue;
        if ((seconds % block->interval) == 0)
            get_block_output(block, &status_bar[i]);
    }
    return;
}

void status_bar_update(bool check_changed) {
    static char status_new[LENGTH(blocks) * (BLOCK_OUTPUT_LENGTH+1)] = {0};
    static char status_old[LENGTH(blocks) * (BLOCK_OUTPUT_LENGTH+1)] = {0};
    char *pointer = status_new;
    if (check_changed)
        memcpy(status_old, status_new, sizeof (status_new));

    for (uint i = 0; i < LENGTH(blocks); i += 1) {
        char *string = status_bar[i].string;
        usize size = status_bar[i].length;
        memcpy(pointer, string, size);
        pointer += size;
    }

    if (check_changed) {
        if (!memcmp(status_old, status_new, sizeof (status_new)))
            return;
    }

    XStoreName(display, root, status_new);
    XFlush(display);
    return;
}

void itos(int num, char *str) {
    int i = 0;
    int isNegative = 0;

    if (num < 0) {
        isNegative = 1;
        num = -num;
    }

    do {
        str[i++] = num % 10 + '0';
        num /= 10;
    } while (num > 0);

    if (isNegative)
        str[i++] = '-';

    str[i] = '\0';

    int length = strlen(str);
    for (int j = 0; j < length / 2; j++) {
        char temp = str[j];
        str[j] = str[length - j - 1];
        str[length - j - 1] = temp;
    }
    return;
}

void signal_handler(int signum) {
    Block *block_updated = NULL;
    for (uint i = 0; i < LENGTH(blocks); i += 1) {
        Block *block = &blocks[i];
        if (block->signal == (signum - SIGRTMIN)) {
            get_block_output(block, &status_bar[i]);
            block_updated = block;
        }
    }
    if (!block_updated) {
        char *msg = "No block configured for signal ";
        char number[20];
        itos(signum - SIGRTMIN, number);

        write(STDERR_FILENO, msg, strlen(msg));
        write(STDERR_FILENO, number, strlen(number));
        write(STDERR_FILENO, ".\n", 2);
        return;
    }
    status_bar_update(true);
    return;
}

void button_block(char *button, Block *block) {
    char *command[2];
    pid_t child;

    switch ((child = fork())) {
    case 0:
        if (block->function) {
            block->function(atoi(button), NULL);
            return;
        }
        command[0] = block->command;
        command[1] = NULL;
        setenv("DWMBLOCKS2_BUTTON", button, 1);
        execvp(command[0], command);
        fprintf(stderr, "Error running %s: %s\n",
                        command[0], strerror(errno));
        exit(EXIT_SUCCESS);
    case -1: {
        char *msg = "Error forking: ";
        char *error = strerror(errno);
        write(STDERR_FILENO, msg, strlen(msg));
        write(STDERR_FILENO, error, strlen(error));
        write(STDERR_FILENO, "\n", 2);
        return;
    }
    default:
        // wait is supposed to fail because
        // signal_childs.sa_flags is set to SA_NOCLDWAIT;
        waitpid(child, NULL, 0);
        kill(getpid(), SIGRTMIN + block->signal);
    }
    return;
}

void button_handler(int signum, siginfo_t *signal_info, void *ucontext) {
    char button[2] = {'0' + (signal_info->si_value.sival_int & 7), '\0'};
    bool any = false;
    (void) ucontext;

    signum = (signal_info->si_value.sival_int >> 3) - SIGRTMIN;
    for (uint i = 0; i < LENGTH(blocks); i += 1) {
        if (blocks[i].signal == signum) {
            button_block(button, &blocks[i]);
            any = true;
        }
    }
    if (!any)
        fprintf(stderr, "No block configured for signal %d.\n", signum);
    return;
}

int popen_no_shell(char *command) {
    int pipefd[2];
    pid_t pid;

    char *argv[2] = { command, NULL };

    if (pipe(pipefd) < 0) {
        fprintf(stderr, "Error creating pipe: %s\n", strerror(errno));
        return -1;
    }

    switch ((pid = fork())) {
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
        return -1;
    default:
        close(pipefd[1]);
        return pipefd[0];
    }
}
