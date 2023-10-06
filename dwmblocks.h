#ifndef DWMBLOCKS_H
#define DWMBLOCKS_H

typedef struct {
	char* icon;
	char* command;
	unsigned int interval;
	int signal;
} Block;

#endif /* DWMBLOCKS_H */
