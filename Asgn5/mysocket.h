#ifndef _MY_SOCKET_H_
#define _MY_SOCKET_H_

#include <sys/socket.h>

// Special flag for message-oriented TCP.
#define SOCK_MyTCP 0
// Ref for other flags: https://codebrowser.dev/glibc/glibc/sysdeps/unix/sysv/linux/bits/socket_type.h.html

// Wrapper to socket system call.
int my_socket(int domain, int type, int protocol);

// Wrapper to bind system call.
int my_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

// Wrapper to listen system call.
int my_listen(int sockfd, int backlog);

// Wrapper to accept system call.
int my_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

// Wrapper to connect system call.
int my_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

// Wrapper to send system call.
ssize_t my_send(int sockfd, const void *buf, size_t len, int flags);

// Wrapper to recv system call.
ssize_t my_recv(int sockfd, void *buf, size_t len, int flags);

// function for thread to send msg waiting in Send_Message queue
void *send_msg(void *arg); 

// fucction for thread to wait on recv call and put the msg into Received_Message
void *recv_msg(void *arg);

// function to receive msg using bufffer of maximum capacity 1000
int receive_chunks(int sockfd, char *result, int size);

// function to send msg using bufffer of maximum capacity 1000
void send_chunks(int new_socket, char *result);

// Wrapper to close system call.
int my_close(int fd);

#endif