#include <fcntl.h>
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

int setnonblocking(int sockfd)
{
    if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0)|O_NONBLOCK) == -1) {
        return -1;
    }
    return 0;
}

int create_udp_fd(uint16_t port)
{
    int listenfd;
    struct sockaddr_in localaddr;
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
    return listenfd;
}
