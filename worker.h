#ifndef WORKER_H
#define WORKER_H

#define MAX_TRANSPORT_FDS 32

struct ControlContent {
    int fd;
    char data[256];
};

struct WorkerContext {
    int c_fd;
    int transport_fd[MAX_TRANSPORT_FDS];
};

struct WorkerContext* init_worker_context();


#endif
