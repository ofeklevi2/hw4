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
    struct Node *prev;
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
    struct cv *prev;
}cv;

typedef struct cv_queue{
    cv *head;
    atomic_size_t waiting;
}cv_queue;

/* //---------- Thread -------

typedef struct thread{
    thrd_t thrd;
    struct thread *next;
}thread;

typedef struct thread_queue{
    atomic_size_t waiting;
    thrd_t *head;
}thread_queue;
//--------------------*/

Queue *queue;
cv_queue *cv_q;
//thread_queue *thread_q;
mtx_t mtx;

void initQueue(void){
    queue = (Queue *)malloc(sizeof(Queue));
    queue->size = 0;
    queue->visited = 0;
    queue->head = NULL;
    queue->tail = NULL;

//   thread_q = (thread_queue *)malloc(sizeof(thread_queue));
//   thread_q->waiting = 0;
//  thread_q->head = NULL;

    cv_q = (cv_queue *)malloc(sizeof(cv_queue));
    cv_q->head = NULL;
    cv_q->waiting = 0;
    mtx_init(&mtx, mtx_plain);
}

void destroyQueue(void){
    mtx_lock(&mtx);
    Node *curr = queue->head;
    Node *tmp = curr;
    while (curr != NULL){
        curr = curr->next;
        free(tmp);
        tmp = curr;
    }

    cv *curr = cv_q->head;
    cv *tmp = curr;
    while (curr != NULL){
        curr = curr->next;
        free(tmp);
        tmp = curr;
    }
    mtx_unlock(&mtx);
    mtx_destroy(&mtx);

/*
    thread *curr = thread_q->head;
    thread *tmp = curr;
    while (curr != NULL){
        curr = curr->next;
        free(tmp);
        tmp = curr;
    }
*/
}

void enqueue(void* item){
    mtx_lock(&mtx);
    Node *new_Node = (Node *)malloc(sizeof(Node));
    new_Node->data = item;
    new_Node->next = NULL;
    new_Node->prev = NULL;

    
    mtx_unlock(&mtx);
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



