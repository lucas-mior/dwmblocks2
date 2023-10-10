#include "dwmblocks2.h"
#include "blocks.h"

#define LENGTH(X) (sizeof (X) / sizeof (*X))
#define BLOCK_OUTPUT_LENGTH 64

typedef struct Output {
    char string[BLOCK_OUTPUT_LENGTH];
    uint32 length;
} Output;

static Display *display;
static Window root;
static Output status_bar[LENGTH(blocks)] = {0};
static Output clock_output = {0};
static int clock_signal;
static const char delim = ' ';

static FILE *popen_no_shell(char *);
static void block_clock(int);
static void button_handler(int, siginfo_t *, void *);
static void get_block_output(const Block *, Output *);
static void get_block_outputs(int64);
static void signal_handler(int);
static void status_bar_update(bool);

int main(void) {
    {
        int sig_max = SIGRTMAX - SIGRTMIN;
        struct sigaction signal_dwm;
        struct sigaction signal_child_action;
        signal_child_action.sa_handler = SIG_DFL;
        signal_child_action.sa_flags = SA_NOCLDWAIT;

        for (int i = SIGRTMIN; i <= SIGRTMAX; i += 1)
            signal(i, SIG_IGN);

        for (uint i = 0; i < LENGTH(blocks); i += 1) {
            Block *block = &blocks[i];
            char *signal_string;

            if (block->environment_variable == NULL) {
                fprintf(stderr, "Error: Signal environmental variable"
                                " must be defined for every block.\n");
                exit(EXIT_FAILURE);
            }
            if ((signal_string = getenv(block->environment_variable)) == NULL) {
                fprintf(stderr, "Error: %s is not defined.\n",
                                block->environment_variable);
                exit(EXIT_FAILURE);
            }

            block->signal = atoi(signal_string);
            if (blocks->signal <= 0) {
                fprintf(stderr, "Invalid signal for block: Must be grater than 0.\n");
                exit(EXIT_FAILURE);
            }
            if (blocks->signal >= sig_max) {
                fprintf(stderr, "Invalid signal for block: Must be lower than %d.\n",
                                sig_max);
                exit(EXIT_FAILURE);
            }
            printf("=======================\n");
            printf("%s %s = %d, %d",
                    block->command, block->environment_variable,
                    block->signal, block->interval);

            signal(SIGRTMIN + block->signal, signal_handler);
            sigaddset(&signal_dwm.sa_mask, SIGRTMIN + block->signal);
            // used by dwm to send proper signal number back to dwmblocks2
            status_bar[i].string[0] = (char) block->signal;
        }
        signal(SIGRTMIN + clock_signal, signal_handler);
        sigaddset(&signal_dwm.sa_mask, SIGRTMIN + clock_signal);

        signal_dwm.sa_sigaction = button_handler;
        signal_dwm.sa_flags = SA_SIGINFO;
        sigaction(SIGUSR1, &signal_dwm, NULL);
        sigaction(SIGCHLD, &signal_child_action, NULL);
    }

    {
        char *DWMBLOCKS2_CLOCK;
        if ((DWMBLOCKS2_CLOCK = getenv("DWMBLOCKS2_CLOCK")) == NULL) {
            fprintf(stderr, "DWMBLOCKS2_CLOCK"
                            "environmental variable is not defined\n.");
            exit(EXIT_FAILURE);
        }
        clock_signal = atoi(DWMBLOCKS2_CLOCK);
        if (clock_signal <= 0) {
            fprintf(stderr, "Invalid signal for block: Must be grater than 0.\n");
            exit(EXIT_FAILURE);
        }
        clock_output.string[0] = (char) clock_signal;
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
    FILE *command_pipe;
    char *status;
    int error;
    char *string = out->string + 1;

    if (!(command_pipe = popen_no_shell(block->command))) {
        fprintf(stderr, "Failed to run %s: %s\n",
                         block->command, strerror(errno));
        string[0] = '\0';
        return;
    }
    do {
        errno = 0;
        status = fgets(string, BLOCK_OUTPUT_LENGTH - 1, command_pipe);
        error = errno;
    } while (!status && error == EINTR);
    // TODO: Check if pclose() is right here, because
    // popen_no_shell uses pipe() and fdopen()
    pclose(command_pipe);
    if (!status) {
        string[0] = '\0';
        out->length = 0;
        return;
    }

    out->length = (uint32) strcspn(string, "\n");
    string[out->length] = '\0';
    if (out->length == 0)
        return;

    if (block->signal == 7) {
        char *msg = "====output of block_music.sh: ====\n";
        write(STDOUT_FILENO, msg, strlen(msg) + 1);
        write(STDOUT_FILENO, string, out->length);
        write(STDOUT_FILENO, "\n", 2);
    }

    while (string[out->length - 1] == delim) {
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
    block_clock(0);
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
    memcpy(pointer, clock_output.string, clock_output.length);

    if (check_changed) {
        if (!memcmp(status_old, status_new, sizeof (status_new)))
            return;
    }

    XStoreName(display, root, status_new);
    XFlush(display);
    return;
}

void signal_handler(int signum) {
    Block *block_updated = NULL;
    for (uint i = 0; i < LENGTH(blocks); i += 1) {
        Block *block = &blocks[i];
        if (block->signal == (signum - SIGRTMIN)) {
            if (block->signal == 7) {
                char *msg = "signaled music\n";
                write(STDOUT_FILENO, msg, strlen(msg) + 1);
            }
            get_block_output(block, &status_bar[i]);
            block_updated = block;
        }
    }
    if (!block_updated) {
        fprintf(stderr, "No block configured for signal %d\n",
                        signum - SIGRTMIN);
        return;
    }
    status_bar_update(true);
    return;
}

void button_handler(int signum, siginfo_t *signal_info, void *ucontext) {
    char button[2] = {'0' + (signal_info->si_value.sival_int & 7), '\0'};
    Block *block = NULL;
    char *command[2];
    pid_t child;
    (void) ucontext;

    signum = (signal_info->si_value.sival_int >> 3) - SIGRTMIN;
    for (uint i = 0; i < LENGTH(blocks); i += 1) {
        if (blocks[i].signal == signum) {
            block = &blocks[i];
            break;
        }
    }
    if (!block) {
        if (signum == clock_signal)
            block_clock(atoi(button));
        else
            fprintf(stderr, "No block configured for signal %d\n", signum);
        return;
    }

    switch ((child = fork())) {
    case 0:
        command[0] = block->command;
        command[1] = NULL;
        setenv("DWMBLOCKS2_BUTTON", button, 1);
        execvp(command[0], command);
        fprintf(stderr, "Error running %s: %s\n",
                        command[0], strerror(errno));
        exit(EXIT_SUCCESS);
    case -1:
        fprintf(stderr, "Error forking: %s\n", strerror(errno));
        return;
    default:
        // wait is supposed to fail because
        // signal_child_action.sa_flags is set to SA_NOCLDWAIT;
        waitpid(child, NULL, 0);
        kill(getpid(), SIGRTMIN + block->signal);
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
        return NULL;
    default:
        close(pipefd[1]);
        return fdopen(pipefd[0], "r");
    }
}

void block_clock(int button) {
    time_t seconds_since_epoch;
    struct tm t;
    char *week;
    char *week_names[] = { "dom", "seg", "ter", "qua", "qui", "sex", "sáb" };
    char *output = clock_output.string + 1;
    int n;

    seconds_since_epoch = time(NULL);
    t = *localtime(&seconds_since_epoch);
    week = week_names[t.tm_wday];

    n = snprintf(output, BLOCK_OUTPUT_LENGTH - 1,
                "📅 %s %02d/%02d %02d:%02d:%02d ",
                 week, t.tm_mday, t.tm_mon + 1, t.tm_hour, t.tm_min, t.tm_sec);
    clock_output.length = (uint32) n + 2;

    switch (button) {
    case 1:
        if (fork() == 0) {
            system("yad --calendar --undecorated --fixed --no-buttons "
                   "| tr -d '\n' | xsel -b");
            exit(EXIT_SUCCESS);
        }
        break;
    case 2:
        if (fork() == 0) {
            execlp("print_screen.sh", "print_screen.sh", "tela", NULL);
            exit(EXIT_SUCCESS);
        }
        break;
    case 3:
        if (fork() == 0) {
            execlp("killall", "killall", "yad", NULL);
            exit(EXIT_SUCCESS);
        }
        break;
    default:
        break;
    }
    return;
}
