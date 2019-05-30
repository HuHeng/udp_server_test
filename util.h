#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>

//for debug
#define dlog(fmt, ...)  \
        fprintf(stderr, "file: %s, line: %d, " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#endif
