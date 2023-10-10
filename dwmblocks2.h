#ifndef DWMBLOCKS2_H
#define DWMBLOCKS2_H

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

#define LENGTH(X) (sizeof (X) / sizeof (*X))
#define BLOCK_OUTPUT_LENGTH 64
#define IS_SPACE(X) ((X == ' ') || (X == '\t') || (X == '\n'))

typedef struct Output {
    char string[BLOCK_OUTPUT_LENGTH];
    uint32 length;
} Output;

typedef struct Block {
    void (*function)(int, Output *);
    char *command;
    char *signal_var_name;
    int interval;
    int signal;
} Block;

extern Block blocks[];

#endif /* DWMBLOCKS2_H */
