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
}cv;

typedef struct cv_queue{
    cv *head;
}cv_queue;

//---------- Thread -------

typedef struct thread{
    thrd_t thrd;
    struct thread *next;
}thread;

typedef struct thread_queue{
    atomic_size_t waiting;
    thrd_t *head;
}thread_queue;
//-------------------


static Queue queue;
static cv_queue cv_q;
static thread_queue thread_q;

void initQueue(void){
    queue.size = 0;
    queue.visited = 0;
    queue.head = NULL;
    queue.tail = NULL;

    thread_q.waiting = 0;
    thread_q.head = NULL;

    cv_q.head = NULL;
}

void destroyQueue(void){
    Node *curr = queue.head;
    Node *tmp = curr;
    while (curr != NULL){
        curr = curr->next;
        free(tmp);
        tmp = curr;
    }

    cv *curr = cv_q.head;
    cv *tmp = curr;
    while (curr != NULL){
        curr = curr->next;
        free(tmp);
        tmp = curr;
    }

    thread *curr = thread_q.head;
    thread *tmp = curr;
    while (curr != NULL){
        curr = curr->next;
        free(tmp);
        tmp = curr;
    }
}

void enqueue(void* item){
    Node *new_node = (Node *)malloc(sizeof(Node));
    new_node->data = item;
    new_node->next = NULL;
    
}

size_t size(void){
    return queue.size;
}

size_t waiting(void){
    return thread_q.waiting;
}

size_t visited(void){
    return queue.visited;
}



