
#include "utils.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

char *trim_whitespace(char *s) {
    if (!s) return s;
    while (isspace((unsigned char)*s)) s++;
    if (*s == 0) return s;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';
    return s;
}

void clear_stdin_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}
