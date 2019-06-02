#include <stdint.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include "util.h"

#define PORT 7000
#define MAX_EVENTS 256
#define BUFLEN 1024
#define WNUM 4

struct Packet {
    struct sockaddr_in peer_addr;
    char msg[BUFLEN];
};

struct Worker {
    pthread_t t_id;
    int cfd; //control fd
    //int listenfd;  //listenfd
    int epollfd;
    int processed_msg_num;
    uint16_t port;
};

struct Server {
    struct Worker* w[WNUM];
    int   cfd[WNUM][2]; 
    int listenfd;
    int epollfd;
    int processed_msg_num;
    uint16_t port;
};

typedef void (*handle_event)(void* arg);

struct EventFunc {
    handle_event handle;
    void* arg;
};

void handle_msg_from_client(void* data)
{
    struct sockaddr_in remote_addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    int fd = (int)data;
    int ret;
    char buf[BUFLEN];

    //print
    memset(buf, 0, BUFLEN);
    ret = recvfrom(fd, &buf, sizeof(buf), 0, (struct sockaddr*)&remote_addr, &addr_len);
    if(ret <= 0) {
        perror("handle fd");
        dlog("read from remote peer: %d\n", ret);
        return;
    }

    dlog("msg from remote %s:%d, %s", inet_ntoa(remote_addr.sin_addr), ntohs(remote_addr.sin_port), buf);
}

void handle_msg_from_server(void* data)
{
    struct Worker* w = (struct Worker*)data;
    struct Packet pkt;

    int ret, listenfd;
    struct epoll_event ev;

    ret = read(w->cfd, &pkt, sizeof(pkt));
    if(ret != sizeof(pkt)) {
        dlog("read from cfd: %d, len %d was not pkt size\n", w->cfd, ret);
        return;
    }

    //get msg and remote addr

    dlog("pkt.msg: %s", pkt.msg);

    listenfd = create_udp_fd(w->port);
    if(listenfd < 0) {
        dlog("create udp fd failed\n");
        return;
    }

    //connect remote peer
    if(connect(listenfd, (struct sockaddr*)&pkt.peer_addr, sizeof(pkt.peer_addr)) == -1) {
        perror("connect");
        dlog("connect remote addr failed\n");
        return;
    }

    //add event
    struct EventFunc* event_func = (struct EventFunc*)malloc(sizeof(struct EventFunc));
    event_func->handle = handle_msg_from_client;
    event_func->arg = (void*)listenfd;

    //add 
    ev.data.ptr = event_func;
    ev.events = EPOLLIN;
    if(epoll_ctl(w->epollfd, EPOLL_CTL_ADD, listenfd, &ev) == -1){
        perror("epoll_ctl");
        dlog("epoll_ctl failed\n");
        return;
    }
}


void* thread_func(void* arg)
{
    int nfds, n;
    struct epoll_event ev, events[MAX_EVENTS];
    struct Worker* w = (struct Worker*)arg;

    dlog("create worker\n");

    w->epollfd = epoll_create1(0);
    if(w->epollfd == -1) {
        perror("epoll_create1");
        dlog("epoll create failed\n");
        return NULL;
    }
    //
    struct EventFunc* event_func = (struct EventFunc*)malloc(sizeof(struct EventFunc));
    event_func->handle = handle_msg_from_server;
    event_func->arg = w;

    //add 
    ev.data.ptr = event_func;
    ev.events = EPOLLIN;
    if(epoll_ctl(w->epollfd, EPOLL_CTL_ADD, w->cfd, &ev) == -1){
        perror("epoll_ctl");
        dlog("epoll_ctl failed\n");
        return NULL;
    }

    for(;;) {
        nfds = epoll_wait(w->epollfd, events, MAX_EVENTS, -1);
        for(n = 0; n < nfds; ++n) {
            struct EventFunc* func = (struct EventFunc*)events[n].data.ptr;
            func->handle(func->arg);
        }

    }
}


struct Worker* create_worker(int cfd, uint16_t port)
{
    struct Worker *w = (struct Worker*)malloc(sizeof(struct Worker));
    int ret;

    memset(w, 0, sizeof(struct Worker));
    w->cfd = cfd;
    w->port = port;
    ret = pthread_create(&w->t_id, NULL, thread_func, w);
    if(ret != 0) {
        dlog("pthread create error, ret: %d\n", ret);
        return NULL;
    }
    return w;
}

struct Server* server_init(uint16_t port) 
{
    struct Server* server = NULL;
    int listenfd;
    int i;

    //
    server = (struct Server*)malloc(sizeof(struct Server));
    memset(server, 0, sizeof(struct Server));
    server->port = port;

    listenfd = create_udp_fd(server->port);
    if(listenfd < 0) {
        dlog("create udp fd failed\n");
        return NULL;
    }
    server->listenfd = listenfd;

    //create socketpair, create worker

    for(i = 0; i < WNUM; ++i) {
        if(socketpair(AF_UNIX, SOCK_DGRAM, 0, server->cfd[i]) < 0){
            perror("socketpair");
            dlog("create socketpair failed, i: %d\n", i);
            return NULL;
        }

        server->w[i] = create_worker(server->cfd[i][1], server->port);
        if(server->w[i] == NULL) {
            dlog("create worker failed, i: %d\n", i);
            return NULL;
        }

    }

    return server;
}


void server_handle_msg(void* data) {
    char buf[BUFLEN];
    int ret, index;
    struct Server* server = (struct Server*)data;
    struct Packet pkt;
    struct sockaddr_in remote_addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);

    dlog("read fd: %d\n", server->listenfd);
    
    memset(buf, 0, BUFLEN);
    ret = recvfrom(server->listenfd, &buf, sizeof(buf), 0, (struct sockaddr*)&remote_addr, &addr_len);
    if(ret <= 0) {
        perror("handle fd");
        dlog("read from remote peer: %d\n", ret);
        return;
    }
    dlog("msg: %s", buf);

    memcpy(pkt.msg, buf, BUFLEN);
    pkt.peer_addr = remote_addr;

    //post to client
    dlog("remote port: %d\n", ntohs(remote_addr.sin_port));

    //hash
    index = ntohs(remote_addr.sin_port) % WNUM;

    //post
    write(server->cfd[index][0], &pkt, sizeof(pkt));

}

void server_run(struct Server* server)
{
    int nfds, n;
    struct epoll_event ev, events[MAX_EVENTS];
    server->epollfd = epoll_create1(0);
    if(server->epollfd == -1) {
        perror("epoll_create1");
        dlog("epoll create failed\n");
        return;
    }

    //
    struct EventFunc* event_func = (struct EventFunc*)malloc(sizeof(struct EventFunc));
    event_func->handle = server_handle_msg;
    event_func->arg = server;

    //add 
    ev.data.ptr = event_func;
    ev.events = EPOLLIN;
    if(epoll_ctl(server->epollfd, EPOLL_CTL_ADD, server->listenfd, &ev) == -1){
        perror("epoll_ctl");
        return;
    }

    //wait
    for(;;) {
        nfds = epoll_wait(server->epollfd, events, MAX_EVENTS, -1);
        for(n = 0; n < nfds; ++n) {
            struct EventFunc* func = (struct EventFunc*)events[n].data.ptr;
            func->handle(func->arg);
        }

    }
}


void run(short port) {
    struct Server* server = server_init(port);
    server_run(server);
}
