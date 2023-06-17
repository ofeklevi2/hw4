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
typedef struct Node{
    void *data;
    struct Node *next;
}Node;

typedef struct Queue{
    atomic_size_t size;
    atomic_size_t visited;
    Node *head;
    Node *tail;
}Queue;

//---------- CV -------

typedef struct cv{
    cnd_t cond;
    struct cv *next;
    Node *Node_to_dq;
}cv;

typedef struct cv_queue{
    cv *head;
    cv *tail;
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
    queue->head = NULL;
    queue->tail = NULL;

    cv_q = (cv_queue *)malloc(sizeof(cv_queue));
    cv_q->head = NULL;
    cv_q->waiting = 0;
    mtx_unlock(&mtx);
    
}

void destroyQueue(void){
    mtx_lock(&mtx);
    Node *curr_Node = queue->head;
    Node *tmp_Node = curr_Node;
    cv *curr_cv = cv_q->head;
    cv *tmp_cv = curr_cv;
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
    cv *curr_cv_q_head;
    Node *new_Node;
    queue->size++;
    //if no thread is waiting to dequeue
    if (cv_q->head == NULL){
        //if head is NULL -> the queue is empty
        if(queue->head == NULL){
            queue->head = (Node *)malloc(sizeof(Node));
            queue->head->data = item;
            queue->head->next = NULL;
            queue->tail = queue->head;
            new_Node = queue->head;
        }
        else{
            new_Node = (Node *)malloc(sizeof(Node));
            new_Node->data = item;
            new_Node->next = NULL;
            queue->tail->next = new_Node;
            queue->tail = queue->tail->next;
        }
    }

    // if cv_q is not null -> connect new_Node to the first cv on cv_q
    else{
        new_Node = (Node *)malloc(sizeof(Node));
        new_Node->data = item;
        new_Node->next = NULL;
        cv_q->head->Node_to_dq = new_Node;
        curr_cv_q_head = cv_q->head;
        cv_q->head = cv_q->head->next;
        cnd_signal(&(curr_cv_q_head->cond));
    }
    mtx_unlock(&mtx);
}

void *dequeue(void){
    mtx_lock(&mtx);
    cnd_t cond;
    cv *new_cv;
    void *item;
    Node *tmp_Node;

    
    //if queue is not empty -> dequeue its first item
    if (queue->head != NULL){
        item = queue->head->data;
        tmp_Node = queue->head;
        queue->head = queue->head->next;
        queue->size--;
        queue->visited++;
        free(tmp_Node);
    }
    //if queue is empty -> put the thread to sleep
    else{
        //init new_cv 
        cnd_init(&cond);
        new_cv = (cv *)malloc(sizeof(cv));
        new_cv->cond = cond;
        new_cv->Node_to_dq = NULL;
        new_cv->next = NULL;

        if (cv_q->head == NULL){
            cv_q->head = new_cv;
            cv_q->tail = new_cv;
        }
        else{
        cv_q->tail->next = new_cv;
        cv_q->tail = cv_q->tail->next;
        }
        cv_q->waiting++;
        cnd_wait(&cond, &mtx); //new_cv is now waiting
        cv_q->waiting--; //new_cv has stopped waiting, which means a new item had been enqueued & new_cv is the next to dequeue
        item = new_cv->Node_to_dq->data;
        /*
        if (item != NULL){
            queue->visited++;
            queue->size--;
        }
        */
        cv_q->head = cv_q->head->next;
        free(new_cv);
    }
    cnd_destroy(&cond);
    mtx_unlock(&mtx);
    return item;
}

bool tryDequeue(void** item){
    mtx_lock(&mtx);
    Node *tmp_Node;
    //if there is an item to dequeue
    if (queue->head != NULL){
        tmp_Node = queue->head;
        queue->head = queue->head->next;
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



