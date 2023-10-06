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

#define LENGTH(X) (sizeof (X) / sizeof (X[0]))
#define CMDLENGTH 100UL

#ifndef DWMBLOCKS_H
#define DWMBLOCKS_H

typedef struct {
	char *command;
	int interval;
	int signal;
} Block;

static Block blocks[] = {
	/*Command*/    /*Update Interval*/    /*Update Signal*/
	{"cat /tmp/ric 2>/dev/null",        0,    10},
	{"clipout.sh",                      0,    11},
	{"0ydl.sh",                         0,    12},
	{"0notmounted.sh",                  0,    13},
	{"0mounted.sh",                     0,    14},
	{"0phone.sh",                       0,    15},
	{"musica.sh",                       0,    16},
	{"vol.sh",                          0,    17},
	{"mic.sh",                          0,    18},
	{"bril.sh",                         0,    19},
	{"rorario",                         1,    21},
	{"1memory.sh",                     60,    22},
	{"1temp.sh",                       60,    23},
	{"1internet.awk"
	 " /sys/class/net/w*/flags"
	 " /sys/class/net/*/operstate",     0,    27},
	{"1bluetooth.sh",                  80,    25},
	{"bateria.awk "
		 "/sys/class/power_*/BAT0/uevent", 60,    26},
};

int get_status(char *, char *);
int greatest_common_denominator(int, int);
void button_handler(int, siginfo_t *, void *);
void get_block_output(const Block *, char *);
void get_block_outputs(int);
void get_signal_commands(int);
void remove_all(char *, char);
void setroot(void);
void setup_signals(void);
void signal_handler(int);
void status_loop(void) __attribute__((noreturn));

extern Display *display;
extern Window root;
extern int screen;

#endif /* DWMBLOCKS_H */
