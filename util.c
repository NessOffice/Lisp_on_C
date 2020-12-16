#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#define MAX_BUFFER_SIZE 2048

char *readline(char *prompt) {
    static char input_buf[MAX_BUFFER_SIZE];

    fputs(prompt, stdout);
    fgets(input_buf, MAX_BUFFER_SIZE, stdin);
    char *cpy = malloc(strlen(input_buf)+1);
    strcpy(cpy, input_buf);
    cpy[strlen(cpy)-1] = '\0';
    return cpy;
}

int println(const char *fmt, ...)
{
    static char printf_buf[MAX_BUFFER_SIZE];
    va_list args;
    int printed;
    va_start(args, fmt);
    printed = vsprintf(printf_buf, fmt, args);
    va_end(args);
    puts(printf_buf);
    return printed;
}
