#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>

#include "queue.h"
//---------- Queue -------
typedef struct Queue{
    atomic_size_t size;
    atomic_size_t visited;
    void *data;
    struct Queue *next;
}Queue;

typedef struct cv_queue{
    void *item_to_dq;
    cnd_t cond;
    struct cv_queue *next;
    atomic_size_t waiting;
}cv_queue;

Queue *queue;
cv_queue *cv_q;
mtx_t mtx;

void initQueue(void){
    mtx_init(&mtx, mtx_plain);
    mtx_lock(&mtx);
    queue = (Queue *)malloc(sizeof(Queue));
    queue->size = 0;
    queue->visited = 0;
    queue->next = NULL;

    cv_q = (cv_queue *)malloc(sizeof(cv_queue));
    cv_q->waiting = 0;
    cv_q->next = NULL;
    mtx_unlock(&mtx);
}

void destroyQueue(void){
    mtx_lock(&mtx);
    Queue *curr_Node = queue;
    Queue *tmp_Node = curr_Node;
    cv_queue *curr_cv = cv_q;
    cv_queue *tmp_cv = curr_cv;

    while (curr_Node != NULL){
        curr_Node = curr_Node->next;
        free(tmp_Node);
        tmp_Node = curr_Node;
    }
    free(queue);


    while (curr_cv != NULL){
        curr_cv = curr_cv->next;
        free(tmp_cv);
        tmp_cv = curr_cv;
    }
    free(cv_q);

    mtx_unlock(&mtx);
    mtx_destroy(&mtx);
}

void enqueue(void *item){
    mtx_lock(&mtx);
    Queue *tmp_queue;
    //cv_queue *tmp_cv;
    cnd_t cnd;
    queue->size++;

    //if no thread is waiting to dequeue
    if (cv_q == NULL){
        //if queue is NULL -> the queue is empty
        if (queue == NULL){
            queue = (Queue *)malloc(sizeof(Queue));
            queue->next = NULL;
            queue->data = item;
        }
        else{
            tmp_queue = queue;
            //we will insert item to the last place in queue
            while (tmp_queue->next != NULL){
                tmp_queue = tmp_queue->next;
            }
            tmp_queue->next = (Queue *)malloc(sizeof(Queue));
            tmp_queue = tmp_queue->next;
            tmp_queue->next = NULL;
            tmp_queue->data = item;
        }
    }
    //if a thread is waiting to dequeue an item - connect the first cv to item and wake it up
    else{
        cv_q->item_to_dq = item;
        cnd = cv_q->cond;
        cv_q = cv_q->next;
        cnd_signal(&cnd);
    }
    mtx_unlock(&mtx);
}

void *dequeue(void){
    mtx_lock(&mtx);
    cnd_t cnd;
    cv_queue *new_cv;
    void *item;
    Queue *tmp_queue;

    cnd_init(&cnd);
    //if queue is not empty -> dequeue its first item
    if (queue != NULL){
        item = queue->data;
        tmp_queue = queue;
        queue = queue->next;
        free(tmp_queue);
        queue->size--;
        queue->visited++;
    }
    //if queue is empty -> put the thread to sleep
    else{
        if(cv_q == NULL){
            cv_q = (cv_queue *)malloc(sizeof(cv_queue));
            cv_q->item_to_dq = NULL;
            cv_q->cond = cnd;
            cv_q->next = NULL;
            new_cv = cv_q;
        }
        else{  
            new_cv = cv_q;
            while (new_cv != NULL){
                new_cv = new_cv->next;
            }
            new_cv->next = (cv_queue *)malloc(sizeof(cv_queue));
            new_cv = new_cv->next;
            new_cv->item_to_dq = NULL;
            new_cv->cond = cnd;
            new_cv->next = NULL;
        }
        cv_q->waiting++;
        cnd_wait(&cnd, &mtx);//new_cv is now waiting
        cv_q->waiting--; //new_cv has stopped waiting, which means a new item had been enqueued & new_cv is the next to dequeue
        item = new_cv->item_to_dq;
        queue->visited++;
        queue->size--;
        free(new_cv);
    }
    cnd_destroy(&cnd);
    mtx_unlock(&mtx);
    return item;
}

bool tryDequeue(void **item){
    mtx_lock(&mtx);
    Queue *tmp_Node;
    //if there is an item to dequeue
    if (queue!= NULL){
        tmp_Node = queue;
        queue= queue->next;
        queue->size--;
        queue->visited++;
        *item = tmp_Node->data;
        free(tmp_Node);
        mtx_unlock(&mtx);
        return true;
    }
    else{
        mtx_unlock(&mtx);
        return false;
    }
}

size_t size(void){
    size_t res;
    mtx_lock(&mtx);
    res = queue->size;
    mtx_unlock(&mtx);
    return res;
}

size_t waiting(void){
    return cv_q->waiting;
}

size_t visited(void){
    return queue->visited;
}

