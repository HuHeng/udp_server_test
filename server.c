#include <stdint.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include "util.h"

#include "worker.h"

#define THREAD_NUM 4
#define PORT 7000
#define MAX_EVENTS 256
#define BUFLEN 1024

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
    int ret = 0, index = 0;
    struct sockaddr_in addr;
    int len = sizeof(addr);
    char buf[BUFLEN];
    memset(buf, 0, BUFLEN);
    //print remote udp ip and port
    if(getpeername(fd, (struct sockaddr*)&addr, &len) < 0) {
        perror("getpeername");
        return -1;
    }

    dlog ("remote machine = %s, port = %d, %d.\n", inet_ntoa(addr.sin_addr), addr.sin_port, ntohs(addr.sin_port));

    //read from remote peer
    ret = read(fd, &buf, sizeof(buf));
    if(ret <= 0) {
        perror("handle fd");
        dlog("read from remote peer: %d\n", ret);
    }

    //post to thread
    index = addr.sin_port % THREAD_NUM;
    dlog("hash to thread id: %d\n", index);

    write(socket)


    return ret;

}

void run(short port, int thread_num)
{
    //create and run
    int i, listenfd;
    int pair[THREAD_NUM][2];

    struct sockaddr_in localaddr;

    int epollfd, nfds, n;
    struct epoll_event ev, events[MAX_EVENTS];

    pthread_t t_id[THREAD_NUM];


    for(i = 0; i < THREAD_NUM; ++i) {
        socketpair(AF_INET, SOCK_DGRAM, 0, pair[i]);
        create_thread(i, pair[i][1], PORT);
    }

    //listen and connect with new fd
    listenfd = socket(PF_INET, SOCK_DGRAM, 0);
    int enable = 1;
    if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt so_reuseaddr");
        return;
    }

    if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) < 0) {
        perror("setsockopt so_reuseport");
        return;
    }

    //set nonblocking
    setnonblocking(listenfd);
    
    memset(&localaddr, 0, sizeof(localaddr));
    localaddr.sin_family = PF_INET;
    localaddr.sin_port = htons(port);
    localaddr.sin_addr.s_addr = INADDR_ANY;
    if(bind(listenfd, (struct sockaddr*)&localaddr, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        return;

    } else {
        dlog("ip and port bind success\n");
    }

    epollfd = epoll_create1(0);

    if(epollfd == -1) {
        perror("epoll_create1");
        return;
    }

    //add c_fd
    ev.data.fd = listenfd;
    ev.events = EPOLLIN;
    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev) == -1){
        perror("epoll_ctl");
        return;
    }

    for(;;) {
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        for(n = 0; n < nfds; ++n) {
            if(events[n].data.fd == listenfd) {
                handle_fd(listenfd);
            }        
        }

    }
    
}
