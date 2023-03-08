#include "mysocket.h"
#include "queue.h"
#include <unistd.h>

Queue Send_Message, Received_Message;

int my_socket(int domain, int type, int protocol) {

    if(protocol == SOCK_MyTCP) {
        initQueue(&Send_Message);
        initQueue(&Received_Message);

        return socket(domain, type, SOCK_STREAM);
    }

    return socket(domain, type, protocol);
}

int my_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return bind(sockfd, addr, addrlen);
}

int my_listen(int sockfd, int backlog) {
    return listen(sockfd, backlog);
}

int my_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    return accept(sockfd, addr, addrlen);
}

int my_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return connect(sockfd, addr, addrlen);
}

ssize_t my_send(int sockfd, const void *buf, size_t len, int flags) {
    Message message;
    message.buf = (char *)malloc(ELE_SIZE);
    message.sockfd = sockfd;
    strcpy(message.buf, buf);
    message.len = len;
    message.flags = flags;

    while(1) {
        if(isQueueFull(&Send_Message)) sleep(2);
        else {
            enqueue(&Send_Message, message);
            return;
        }
    }
}

ssize_t my_recv(int sockfd, void *buf, size_t len, int flags) {
    Message message;
    message.buf = (char *)malloc(ELE_SIZE);
    while(1) {
        if(isQueueEmpty(&Received_Message)) sleep(2);
        else {
            message = dequeue(&Received_Message);
            buf = message.buf;
            return message.len;
        }
    }
}

int my_close(int fd) {
    destroyQueue(&Send_Message);
    destroyQueue(&Received_Message);
    return close(fd);
}