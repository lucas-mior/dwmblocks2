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

#ifndef DWMBLOCKS_H
#define DWMBLOCKS_H

#define LENGTH(X) (sizeof (X) / sizeof (X[0]))
#define BLOCK_OUTPUT_LENGTH 64

typedef struct {
    char *command;
    int interval;
    int signal;
} Block;

int status_update(void);
int gcd(int, int);
void button_handler(int, siginfo_t *, void *);
void get_block_output(const Block *, char *);
void get_block_outputs(int);
void set_root(void);
void signal_handler(int);
FILE *popen_no_shell(char *);

extern Display *display;
extern Window root;

#endif /* DWMBLOCKS_H */
