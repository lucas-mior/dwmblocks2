#include "dwmblocks.h"
#include "blocks.h"

Display *display;
Window root;
static char status_bar[LENGTH(blocks)][BLOCK_OUTPUT_LENGTH] = {0};

char clock_output[BLOCK_OUTPUT_LENGTH] = {0};
int clock_signal;

static char status_new[sizeof (status_bar) + sizeof (clock_output)];
static char status_old[sizeof (status_bar) + sizeof (clock_output)];
static const char delim = ' ';

void get_block_output(const Block *block, char *output) {
    FILE *command_pipe;
    char *status;
    int error;
    size_t length;

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
    time_t ti;
    struct tm t;
    char *week;
    char *output = clock_output + 1;

    (void) time(&ti);
    t = *localtime(&ti);
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
    case 0:
        break;
    case 1:
        system("yad --calendar --undecorated --fixed --no-buttons "
               "| tr -d '\n' | xsel -b");
        break;
    case 2:
        system("print_screen.sh tela");
        break;
    case 3:
        system("killall yad");
        break;
    default:
        break;
    }
    return;
}
