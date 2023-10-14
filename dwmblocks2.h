#ifndef DWMBLOCKS2_H
#define DWMBLOCKS2_H

#include <X11/Xlib.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

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

typedef size_t usize;
typedef ssize_t isize;
#endif

#define LENGTH(X) ((isize) (sizeof (X) / sizeof (*X)))
#define BLOCK_OUTPUT_LENGTH 60
#define IS_SPACE(X) ((X == ' ') || (X == '\t') || (X == '\n'))

typedef struct Block {
    void (*function)(int, struct Block *);
    char *command;
    char *signal_var_name;
    int interval;
    int signal;
    int *pipe;
    char output[BLOCK_OUTPUT_LENGTH];
    int length;
    sigset_t mask;
} Block;

extern Block blocks[];

#endif /* DWMBLOCKS2_H */
