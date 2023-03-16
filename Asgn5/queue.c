#include "queue.h"

#include <stdlib.h>

void initQueue(Queue* queue) {
    queue->front = -1;
    queue->rear = -1;
    for(int i = 0; i < SIZE; i++) {
        queue->queue_arr[i] = (Message *)malloc(sizeof(Message));
        queue->queue_arr[i]->buf = (char *)malloc(ELE_SIZE);
    }
}

// return 1 if queue is empty, else return 0
int isQueueEmpty(Queue* queue)
{
   return (queue->front == -1) ? 1 : 0;
}

// return 1 if queue is full, else return 0
int isQueueFull(Queue* queue)
{
   return ((queue->front == queue->rear + 1) || (queue->front == 0 && queue->rear == SIZE - 1)) ? 1 : 0; 
}

//return the front element and removes it, returns -1 if queue is empty
Message dequeue(Queue* queue)
{   
    if(isQueueEmpty(queue) == 1) {
        Message null;
        return null;
    }
    else
    {
        Message element;
        element.buf = (char *)malloc(ELE_SIZE);
        element.len = queue->queue_arr[queue->front]->len;
        element.flags = queue->queue_arr[queue->front]->flags;
        strcpy(element.buf, queue->queue_arr[queue->front]->buf);
        if(queue->front == queue->rear) {
            queue->front = -1;
            queue->rear = -1;
        }
        else {
            queue->front = (queue->front + 1) % SIZE;
        }
        return element;
    }
}

//add element at rear end
void enqueue(Queue *queue, Message ele)
{
   if(isQueueFull(queue) == 1) return;
   else
   {
       if(isQueueEmpty(queue)) queue->front = 0;
    //    printf("[queue] queue front: %d\n", queue->front);
       queue->rear = (queue->rear + 1) % SIZE;
    //    printf("[queue] queue rear: %d\n", queue->rear);
       queue->queue_arr[queue->rear]->len = ele.len;
    //    printf("[queue] element len: %d\n", queue->queue_arr[queue->rear]->len);
       queue->queue_arr[queue->rear]->flags = ele.flags;
    //    printf("[queue] element flags: %d\n", queue->queue_arr[queue->rear]->flags);
       queue->queue_arr[queue->rear]->buf = (char *)malloc(ele.len);
       strcpy(queue->queue_arr[queue->rear]->buf, ele.buf);
    //    printf("[queue] element buf: %s\n", queue->queue_arr[queue->rear]->buf);
   }
}

void destroyQueue(Queue *queue) {
    for(int i = queue->front; i < queue->rear; i++) {
        free(queue->queue_arr[i]->buf);
        // free(queue->queue_arr[i]);
    }
}