#include "dwmblocks.h"
#include "blocks.h"

#define LENGTH(X) (sizeof (X) / sizeof (*X))
#define BLOCK_OUTPUT_LENGTH 64

static Display *display;
static Window root;
static char status_bar[LENGTH(blocks)][BLOCK_OUTPUT_LENGTH] = {0};

static char clock_output[BLOCK_OUTPUT_LENGTH] = {0};
static int clock_signal;

static char status_new[sizeof (status_bar) + sizeof (clock_output)];
static char status_old[sizeof (status_bar) + sizeof (clock_output)];
static const char delim = ' ';

static void button_handler(int, siginfo_t *, void *);
static void get_block_output(const Block *, char *);
static void get_block_outputs(int64);
static void set_root(bool);
static void signal_handler(int);
static FILE *popen_no_shell(char *);
static void block_clock(int);

int main(void) {
    int64 seconds = -1;
    struct timespec sleep_time;
    struct timespec to_sleep;
    int screen;
    char *HORARIO;

    struct sigaction signal_action;
    struct sigaction signal_child_action;
    signal_child_action.sa_handler = SIG_DFL;
    signal_child_action.sa_flags = SA_NOCLDWAIT;

    if ((HORARIO = getenv("HORARIO")) == NULL) {
        fprintf(stderr, "HORARIO environmental variable is not defined\n.");
    }
    clock_signal = atoi(HORARIO);
    clock_output[0] = (char) clock_signal;

    if ((display = XOpenDisplay(NULL)) == NULL) {
        fprintf(stderr, "Error opening X display\n");
        exit(EXIT_FAILURE);
    }
    screen = DefaultScreen(display);
    root = RootWindow(display, screen);

    for (int i = SIGRTMIN; i <= SIGRTMAX; i += 1)
        signal(i, SIG_IGN);

    for (uint i = 0; i < LENGTH(blocks); i += 1) {
        if (blocks[i].signal > 0) {
            signal(SIGRTMIN + blocks[i].signal, signal_handler);
            sigaddset(&signal_action.sa_mask, SIGRTMIN + blocks[i].signal);
        }
    }
    signal(SIGRTMIN + clock_signal, signal_handler);
    sigaddset(&signal_action.sa_mask, SIGRTMIN + clock_signal);

    signal_action.sa_sigaction = button_handler;
    signal_action.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR1, &signal_action, NULL);
    sigaction(SIGCHLD, &signal_child_action, NULL);

    sleep_time.tv_sec = 1;
    sleep_time.tv_nsec = 0;

    do {
        to_sleep = sleep_time;
        get_block_outputs(seconds);
        set_root(false);

        while (nanosleep(&to_sleep, &to_sleep) < 0);
        seconds += 1;
    } while (true);
}

void get_block_output(const Block *block, char *output) {
    FILE *command_pipe;
    char *status;
    int error;
    usize length;

    if (block->signal) {
        // this messages dwm which signal belongs to which block
        output[0] = (char) block->signal;
        output += 1;
    }
    if (!(command_pipe = popen_no_shell(block->command))) {
        fprintf(stderr, "Failed to run %s: %s\n",
                         block->command, strerror(errno));
        output[0] = '\0';
        return;
    }
    do {
        errno = 0;
        status = fgets(output, BLOCK_OUTPUT_LENGTH - 1, command_pipe);
        error = errno;
    } while (!status && error == EINTR);
    // TODO: Check if pclose() is right here, because
    // popen_no_shell uses pipe() and fdopen()
    pclose(command_pipe);

    length = strcspn(output, "\n");
    output[length] = '\0';
    if (length == 0)
        return;

    while (output[length - 1] == delim) {
        output[length - 1] = '\0';
        length -= 1;
        if (length == 1)
            break;
    }
    if (length > 0) {
        output[length] = delim;
        output[length + 1] = '\0';
    }
    return;
}

void get_block_outputs(int64 seconds) {
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
    block_clock(0);
    return;
}

void set_root(bool check_changed) {
    if (check_changed)
        memcpy(status_old, status_new, sizeof (status_new));

    status_new[0] = '\0';
    for (uint i = 0; i < LENGTH(blocks); i += 1)
        strcat(status_new, status_bar[i]);
    strcat(status_new, clock_output);

    if (check_changed && !memcmp(status_old, status_new, sizeof (status_new))) {
        fprintf(stderr, "Status bar not changed!\n");
        return;
    }

    XStoreName(display, root, status_new);
    XFlush(display);
    return;
}

void signal_handler(int signum) {
    for (uint i = 0; i < LENGTH(blocks); i += 1) {
        Block *block = &blocks[i];
        if (block->signal == (signum - SIGRTMIN))
            get_block_output(block, status_bar[i]);
    }
    set_root(true);
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
        setenv("BLOCK_BUTTON", button, 1);
        execvp(command[0], command);
        fprintf(stderr, "running %s failed: %s\n", command[0], strerror(errno));
        exit(EXIT_SUCCESS);
    case -1:
        fprintf(stderr, "fork failed: %s\n", strerror(errno));
        return;
    default:
        // wait is supposed to fail because of signal_child_action
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
    char *output = clock_output + 1;

    seconds_since_epoch = time(NULL);
    t = *localtime(&seconds_since_epoch);
    week = ((char *[]) {
        "dom", 
        "seg",
        "ter",
        "qua",
        "qui",
        "sex",
        "sáb"
    })[t.tm_wday];

    snprintf(output, BLOCK_OUTPUT_LENGTH - 1, "📅 %s %02d/%02d %02d:%02d:%02d\n",
             week, t.tm_mday, t.tm_mon + 1, t.tm_hour, t.tm_min, t.tm_sec);

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
