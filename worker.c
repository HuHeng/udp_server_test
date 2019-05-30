#include <pthread.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

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


int handle_control_fd(int c_fd, int epollfd) {
    //
    int ret;
    char buf[BUFLEN];
    int listenfd;

    ControlContent cc;
    struct sockaddr_in addr;
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
    if(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt");
        return -1;
    }


    //add epoll


    return ret;
}

//c_fd was socketpair
int worker_func(int c_fd) 
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
            if(event[n].data.fd == c_fd) {
                handle_control_fd(c_fd, epollfd);
            } else {
            
            }
        }
    }

    return 0;
}
