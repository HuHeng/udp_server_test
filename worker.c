#include <pthread.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <string.h>

#include "worker.h"
#include "util.h"

#define MAX_EVENTS 256
#define BUFLEN 1024


void print_ipv4(struct sockaddr *s)
{
    struct sockaddr_in *sin = (struct sockaddr_in *)s;
    char ip[INET_ADDRSTRLEN];
    uint16_t port;

    inet_pton(AF_INET, sin->sin_addr, ip);
    port = htons (sin->sin_port);

    printf ("host %s:%d\n", ip, port);
}

int setnonblocking(int sockfd)
{
    if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0)|O_NONBLOCK) == -1) {
        return -1;
    }
    return 0;
}

int add_event(int epollfd, int fd, int state) {
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    return epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
}


int handle_control_fd(int c_fd, int epollfd, uint16_t port) {
    //
    int ret;
    char buf[BUFLEN];
    int listenfd;

    ControlContent cc;
    struct sockaddr_in addr, localaddr;
    int len = sizeof(addr);


    //read dgram
    ret = read(c_fd, &buf, sizeof(cc));

    dlog("recv data length: %d, cc size: %ld\n", ret, sizeof(cc));

    if(ret != sizeof(cc)) {
        dlog("recv data not expect\n");
        return -1;
    }

    if(cc.fd < 0) {
        dlog("cc.fd: %d\n", cc.fd);
        return -1;
    }
    //print remote udp ip and port
    if(getpeername(cc.fd, (struct sockaddr*)&addr, &len) < 0) {
        perror("getpeername");
        return -1;
    }

    dlog ("remote machine = %s, port = %d, %d.\n", inet_ntoa(addr.sin_addr), addr.sin_port, ntohs(addr.sin_port));

    //listen and connect with new fd
    listenfd = socket(PF_INET, SOCK_DGRAM, 0);
    int enable = 1;
    if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt so_reuseaddr");
        return -1;
    }

    if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) < 0) {
        perror("setsockopt so_reuseport");
        return -1;
    }

    //set nonblocking
    setnonblocking(listenfd);

    memset(&localaddr, 0, sizeof(localaddr));
    localaddr.sin_family = PF_INET;
    localaddr.sin_port = htons(port);
    localaddr.sin_addr.s_addr = INADDR_ANY;
    if(bind(listenfd, (struct sockaddr*)&localaddr, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        return -1;

    } else {
        dlog("ip and port bind success\n");
    }

    //connect remote peer
    if(connect(listenfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        return -1;
    }

    //add epoll
    if(add_event(epollfd, listenfd, EPOLLIN) == -1) {
        perror("add event");
        return -1;
    }

    return ret;
}

int handle_transport_fd(int fd)
{
    //read dgram
    char buf[BUFLEN];
    memset(buf, 0, BUFLEN);

    int ret = read(fd, &buf, sizeof(buf));


    dlog("ret: %d\n", ret);

    if(ret <= 0) {
        return ret;
    }

    dlog("buf: %s\n", buf);

    
}

//c_fd was socketpair
int worker_func(int c_fd, uint16_t port) 
{
    int epollfd, nfds, n;
    struct epoll_event ev, events[MAX_EVENTS];

    epollfd = epoll_create1(0);

    if(epollfd == -1) {
        perror("epoll_create1");
        return -1;
    }

    //add c_fd
    ev.data.fd = c_fd;
    ev.events = EPOLLIN;
    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, c_fd, &ev) == -1){
        perror("epoll_ctl");
        return -1;
    }

    for(;;) {
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        for(n = 0; n < nfds; ++n) {
            if(events[n].data.fd == c_fd) {
                handle_control_fd(c_fd, epollfd, port);

            } else {
                handle_transport_fd(events[n].data.fd);
            }
        }
    }

    return 0;
}
