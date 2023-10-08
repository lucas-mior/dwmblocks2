#include <X11/Xlib.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#ifndef DWMBLOCKS_H
#define DWMBLOCKS_H

#ifndef INTEGERS
#define INTEGERS
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long ulonglong;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
#endif

#define LENGTH(X) (sizeof (X) / sizeof (*X))
#define BLOCK_OUTPUT_LENGTH 64

typedef struct {
    char *command;
    int interval;
    int signal;
} Block;

void button_handler(int, siginfo_t *, void *);
void get_block_output(const Block *, char *);
void get_block_outputs(int64);
void set_root(bool);
void signal_handler(int);
FILE *popen_no_shell(char *);
void block_clock(int);

extern Display *display;
extern Window root;
extern int clock_signal;
extern char clock_output[BLOCK_OUTPUT_LENGTH];

#endif /* DWMBLOCKS_H */
