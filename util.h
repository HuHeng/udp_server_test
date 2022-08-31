#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdint.h>


//for debug
#define dlog(fmt, ...)  \
        fprintf(stderr, "thread: %lu, file: %s, line: %5d, " fmt "\n", syscall(SYS_gettid), __FILE__, __LINE__, ##__VA_ARGS__)

int setnonblocking(int sockfd);

int create_udp_fd(uint16_t port);

#endif
