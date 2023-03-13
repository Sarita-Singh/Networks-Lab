#include "mysocket.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/poll.h>
#include <pthread.h>

Queue Send_Message, Received_Message;
int newsockfd, isClient = 0, isAccepted = 0, isConnected = 0;
pthread_t S;
pthread_mutex_t lock_send_msg;
pthread_cond_t S_cond;

int my_socket(int domain, int type, int protocol)
{

    if (type == SOCK_MyTCP)
    {
        initQueue(&Send_Message);
        initQueue(&Received_Message);

        pthread_attr_t attr;
        int sockfd = socket(domain, SOCK_STREAM, protocol);
        newsockfd = sockfd;
        // explicitly creating threads in a joinable state
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

        // initialize pthread mutex protecting Send_Message and condition variable objects
        pthread_cond_init(&S_cond, NULL);
        pthread_mutex_init(&lock_send_msg, NULL);
        if ((pthread_create(&S, &attr, send_msg, &sockfd)))
        {
            perror("pthread_create\n");
            exit(0);
        }
        // if ((pthread_create(&R, &attr, recv_msg, &sockfd)))
        // {
        //     perror("pthread_create\n");
        //     exit(0);
        // }
        return sockfd;
    }

    return socket(domain, type, protocol);
}

int my_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    return bind(sockfd, addr, addrlen);
}

int my_listen(int sockfd, int backlog)
{
    return listen(sockfd, backlog);
}

int my_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    newsockfd = accept(sockfd, addr, addrlen);
    printf("[mysocket] new socket created : %d\n", newsockfd);
    isAccepted = 1;
    return newsockfd;
}

int my_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    isClient = 1;
    isConnected = 1;
    return connect(sockfd, addr, addrlen);
}

ssize_t my_send(int sockfd, const void *buf, size_t len, int flags)
{
    Message message;
    message.buf = (char *)malloc(ELE_SIZE);
    message.sockfd = sockfd;
    strcpy(message.buf, buf);
    message.len = len;
    message.flags = flags;
    // printf("[mysocket] calling send chunks function\n");
    // send_chunks(sockfd, buf);
    // printf("[mysocket] sent the message over tcp\n");

    if (pthread_mutex_lock(&lock_send_msg))
    {
        perror("[mysocket] send queue lock failed\n");
        exit(1);
    }
    else
    {
        printf("[mysocket] %d main thread locked the send msg queue\n", sockfd);
    }
    if (isQueueFull(&Send_Message))
    {
        printf("[mysocket] %d queue is full. Going to sleep...\n", sockfd);
        sleep(2);
    }
    else
    {
        printf("[mysocket] %d Going to enqueue\n", sockfd);
        enqueue(&Send_Message, message);
        printf("[mysocket] %d message enqueued\n", sockfd);
        printf("[mysocket] %d message of len %d has been enqueued\n", sockfd, message.len);
    }
    if (pthread_mutex_unlock(&lock_send_msg))
    {
        perror("[mysocket] send queue unlock failed\n");
        exit(1);
    }
    else
    {
        printf("[mysocket] %d main thread unlocked the send msg queue\n", sockfd);
    }
    pthread_cond_signal(&S_cond);
    //pthread_exit(NULL);
    return len;
}

ssize_t my_recv(int sockfd, void *buf, size_t len, int flags)
{

    // Message message;
    // message.buf = (char *)malloc(ELE_SIZE);
    int n = receive_chunks(sockfd, buf, len);
    printf("[mysocket] received message string: %s\n", buf + 4070);
    // strcpy(message.buf, buf);
    printf("[mysocket] received the message of len %d\n", strlen(buf));
    // while(1) {
    //     if(isQueueEmpty(&Received_Message)) sleep(2);
    //     else {
    //         message = dequeue(&Received_Message);
    //         buf = message.buf;
    //         printf("[mysocket] dequeued the message of len %d\n", message.len);
    //         return message.len;
    //     }
    // }
}

void *send_msg(void *arg)
{
    // thread S checks periodically if any msg is waiting to be sent
    int sockfd = *(int *)arg;
    printf("[mysocket] socket: %d\n", sockfd);

    while (1)
    {
        // check if send queue has message waiting
        if ((isClient == 0 && isAccepted == 1) || (isClient == 1 && isConnected == 1))
        {
            if (pthread_mutex_lock(&lock_send_msg))
            {
                perror("[mysocket] send queue lock failed\n");
                exit(1);
            }
            else
            {
                printf("[mysocket] %d s thread locked the send msg queue\n", sockfd);
            }
            while (!isQueueEmpty(&Send_Message))
            {
                Message message = dequeue(&Send_Message);
                char buf[ELE_SIZE];
                memset(buf, '\0', ELE_SIZE);
                strcpy(buf, message.buf);
                int newsockfd = message.sockfd;
                printf("[mysocket] %d buf: %s\n", newsockfd, message.buf);
                printf("[mysocket] calling send chunks function\n");
                send_chunks(newsockfd, message.buf);
                printf("[mysocket] %d thread S sent the message over tcp\n", newsockfd);
            }
            if (pthread_mutex_unlock(&lock_send_msg))
            {
                perror("[mysocket] send queue unlock failed\n");
                exit(1);
            }
            else
            {
                printf("[mysocket] %d s thread unlocked the send msg queue\n", sockfd);
            }
        }
        printf("[mysocket] %d s thread going to sleep for 2s\n", sockfd);
        sleep(2);
    }
    pthread_exit(NULL);
}

void *recv_msg(void *arg)
{
    // thread R waits on recv call and put the msg into Received_Message
    int sockfd = *(int *)arg;
    // struct pollfd fds[1];
    // fds[0].fd = sockfd;
    // fds[0].events = POLLIN;
    char buf[5000];
    memset(buf, '\0', ELE_SIZE);
    while (1)
    {
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
    printf("[mysocket] message size: %s\n", result);
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

int my_close(int fd)
{
    // has been mentioned to make this function sleep for 5 sec first
    sleep(5);
    pthread_join(S, NULL);
    // Clean up and exit
    pthread_mutex_destroy(&lock_send_msg);
    pthread_exit(NULL);
    destroyQueue(&Send_Message);
    printf("[mysocket] destroyed send message queue\n");
    destroyQueue(&Received_Message);
    printf("[mysocket] closing socket: %d\n", fd);
    isAccepted = 0;
    isConnected = 0;
    isClient = 0;
    return close(fd);
}