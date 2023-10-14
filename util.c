#include "dwmblocks2.h"
#include "util.h"

char *itoa(int num, char *str) {
    int i = 0;
    bool negative = false;
    usize length;

    if (num < 0) {
        negative = true;
        num = -num;
    }

    do {
        str[i++] = num % 10 + '0';
        num /= 10;
    } while (num > 0);

    if (negative)
        str[i++] = '-';

    str[i] = '\0';

    length = strlen(str);
    for (usize j = 0; j < length / 2; j++) {
        char temp = str[j];
        str[j] = str[length - j - 1];
        str[length - j - 1] = temp;
    }
    return str;
}

void write_error(char *msg) {
    write(STDERR_FILENO, msg, strlen(msg));
}
