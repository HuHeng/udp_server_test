#include "util.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>

#define NUM 2
#define BUFLEN 256

int main(int argc, char** argv) 
{
    uint16_t port = 50000;
    uint16_t server_port = 7000;
    int i, n;
    int fd[NUM];
    int msg_num = 2;
    char buf[BUFLEN];
    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    //port
    for(i = 0; i < NUM; ++i) {
        fd[i] = create_udp_fd(port + i);
        if(fd[i] < 0) {
            dlog("create udp fd failed\n");
        }
    }


    for(n = 0; n < msg_num; ++n){
        for(i = 0; i < NUM; ++i) {
            memset(buf, 0, BUFLEN);
            sprintf(buf, "i: %d, port: %d, n: %d", i, port + i, n);
            sendto(fd[i], buf, strlen(buf), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
            usleep(1000000);
        }
    }

    return 0;

}
