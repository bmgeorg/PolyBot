#include "utility.h"

#include <stdio.h>
#include <stdlib.h>

void _quit(const char *fmt, ...) {
    va_list arg;
    va_start(arg, fmt);
    vfprintf(stderr, fmt, arg);
    va_end(arg);
    exit(1);
}

void _plog(const char *fmt, ...) {
#ifdef DEBUG
    va_list arg;
    va_start(arg, fmt);
    vfprintf(stderr, fmt, arg);
    va_end(arg);
#endif
}