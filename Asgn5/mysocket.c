#include "mysocket.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/poll.h>
#include <pthread.h>

Queue Send_Message, Received_Message;
int newsockfd;

int my_socket(int domain, int type, int protocol) {

    if(type == SOCK_MyTCP) {
        // initQueue(&Send_Message);
        // initQueue(&Received_Message);
        // pthread_t R, S;
        // pthread_attr_t attr;
        int sockfd = socket(domain, SOCK_STREAM, protocol);
        newsockfd = sockfd;
        // explicitly creating threads in a joinable state 
        // pthread_attr_init(&attr);
        // pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        // if ((pthread_create(&S, &attr, send_msg, &sockfd)))
        // {
        //     perror("pthread_create\n");
        //     exit(0);
        // }
        // if ((pthread_create(&R, &attr, recv_msg, &sockfd)))
        // {
        //     perror("pthread_create\n");
        //     exit(0);
        // }
        return sockfd;
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
    newsockfd = accept(sockfd, addr, addrlen);
    printf("[mysocket] new socket created : %d\n", newsockfd);
    return newsockfd;
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
    printf("[mysocket] calling send chunks function\n");
    send_chunks(sockfd, buf);
    printf("[mysocket] sent the message over tcp\n");
    return len;
    while(1) {
        if(isQueueFull(&Send_Message)) sleep(2);
        else {
            enqueue(&Send_Message, message);
            printf("[mysocket] message of len %d has been enqueued\n", len);
            return len;
        }
    }
}

ssize_t my_recv(int sockfd, void *buf, size_t len, int flags) {
    
    Message message;
    message.buf = (char *)malloc(ELE_SIZE);
    receive_chunks(sockfd, buf, len);
    return len;
    // while(1) {
    //     if(isQueueEmpty(&Received_Message)) sleep(2);
    //     else {
    //         message = dequeue(&Received_Message);
    //         buf = message.buf;
    //         printf("\n[mysocket] dequeued the message of len %d", message.len);
    //         return message.len;
    //     }
    // }
}

void *send_msg(void *arg) {
    // thread S checks periodically if any msg is waiting to be sent
    while (1) {
        // check if send queue has message waiting
        while (!isQueueEmpty(&Send_Message)){
            Message message = dequeue(&Send_Message);
            char *buf = message.buf;
            int newsockfd = message.sockfd;
            printf("[mysocket] calling send chunks function\n");
            send_chunks(newsockfd, buf);
            printf("[mysocket] thread S sent the message over tcp\n");
        }
        sleep(2);
    }
}

void *recv_msg(void *arg) {
    // thread R waits on recv call and put the msg into Received_Message
    int sockfd = *(int *)arg;
    // struct pollfd fds[1];
    // fds[0].fd = sockfd;
    // fds[0].events = POLLIN;
    char buf[5000];
    memset(buf, '\0', ELE_SIZE);
    while(1) {
        // int ret = poll(fds, 1, 3000);
        // if (ret == -1) {
        //     perror("Poll\n");
        // }
        // else if(ret == 0) {
        //     printf("\n[mysocket] Timeout");
        // }
        // else {
            // receive using receive_chunks 
            if (receive_chunks(newsockfd, buf, ELE_SIZE) < 0)
            {
                perror("\nError in receiving\n");
                exit(0);
            }
            // now put the received message in Received_Message
            Message message;
            message.buf = (char *)malloc(sizeof(buf));
            message.sockfd = sockfd;
            strcpy(message.buf, buf);
            message.len = strlen(buf);
            // message.flags = flags;
            enqueue(&Received_Message, message);
            printf("[mysocket] thread R received the message over tcp\n");
            sleep(2);
        // }
    }

}

int receive_chunks(int sockfd, char *result, int size)
{
    int n, total = 0;
    while (1)
    {
        char temp[1002];
        memset(&temp, '\0', 1002);
        n = recv(sockfd, temp, 1000, 0);
        if (n < 0)
        {
            perror("Unable to read from socket");
            return -1;
        }
        if (n == 0)
        {
            perror("Connection closed by client");
            return -1;
        }
        total += n;
        long long l = strlen(temp);
        if (l + n > size)
        {
            size = l + n + 1;
            result = (char *)realloc(result, size);
        }
        strcat(result, temp);
        if (temp[n - 1] == '\0')
            break;
    }
    return total;
}

void send_chunks(int new_socket, char *result)
{
    char buffersend[1002];
    int res_len = strlen(result);
    printf("[mysocket] message size: %d\n", res_len);
    result[res_len] = '\0';
    int count = 0;
    res_len++;
    for (int i = 0; i < res_len; i += 1000)
    {
        count = 0;
        memset(&buffersend, '\0', 1002);
        for (int j = 0; j < 1000 && i + j < res_len; j++)
        {
            count++;
            buffersend[j] = result[i + j];
        }
        send(new_socket, buffersend, count, 0);
    }
}

int my_close(int fd) {
    // destroyQueue(&Send_Message);
    // destroyQueue(&Received_Message);
    // pthread_exit(NULL);
    printf("[mysocket] closing socket: %d\n", fd);
    return close(fd);
}