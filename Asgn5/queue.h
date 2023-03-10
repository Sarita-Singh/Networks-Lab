#ifndef _QUEUE_H
#define _QUEUE_H

#include <stdio.h>
#include <string.h>

// size of the queue
#define SIZE 10

// max size of one element
#define ELE_SIZE 5000

typedef struct _message {
    int sockfd;
    char* buf;
    size_t len;
    int flags;
} Message;

typedef struct _queue {
    Message* queue_arr[SIZE];
    int front;
    int rear;
} Queue;

void initQueue(Queue *queue);
int isQueueEmpty(Queue *queue);
int isQueueFull(Queue *queue);
Message dequeue(Queue *queue);
void enqueue(Queue *queue, Message);
void destroyQueue(Queue *queue);

#endif