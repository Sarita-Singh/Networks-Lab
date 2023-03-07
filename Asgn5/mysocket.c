#include "mysocket.h"

int my_socket(int domain, int type, int protocol) {
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
    return send(sockfd, buf, len, flags);
}

ssize_t my_recv(int sockfd, void *buf, size_t len, int flags) {
    return recv(sockfd, buf, len, flags);
}

int my_close(int fd) {
    return close(fd);
}