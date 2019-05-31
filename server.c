#include <stdint.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "worker.h"

#define THREAD_NUM 4
#define PORT 7000

struct WorkerArgs{
    int c_fd;
    uint16_t port;
};

static void* thread_func(void* argv) 
{

    return NULL;
}

void create_thread(int thread_index, int fd, uint16_t port)
{

}

int handle_fd(int fd) 
{

}

void run(short port, int thread_num)
{
    //create and run
    int i;
    int pair[THREAD_NUM][2];


    for(i = 0; i < THREAD_NUM; ++i) {
        socketpair(AF_INET, SOCK_DGRAM, 0, pair[i]);
        create_thread(i, pair[i][1], PORT);
    }

    
    


    //
    //
    
}
