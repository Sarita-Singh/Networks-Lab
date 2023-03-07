#include "mysocket.h"

int my_socket(int domain, int type, int protocol) {
    return 0;
}

int my_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return 0;
}

int my_listen(int sockfd, int backlog) {
    return 0;
}

int my_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    return 0;
}

int my_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return 0;
}

ssize_t my_send(int sockfd, const void *buf, size_t len, int flags) {
    return 0;
}

ssize_t my_recv(int sockfd, void *buf, size_t len, int flags) {
    return 0;
}

int my_close(int fd) {
    return 0;
}