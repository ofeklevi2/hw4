#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <stdatomic.h>
#include <stdbool.h>

// Node structure for the linked list
typedef struct Node {
    void* data;
    struct Node* next;
} Node;

typedef struct {
    Node* head;
    Node* tail;
    mtx_t mutex;
    cnd_t not_empty;
    atomic_size_t num_waiting;
    atomic_size_t num_visited;
} ConcurrentQueue;

static ConcurrentQueue queue;

void initQueue(void) {
    queue.head = NULL;
    queue.tail = NULL;
    mtx_init(&queue.mutex, mtx_plain);
    cnd_init(&queue.not_empty);
    atomic_init(&queue.num_waiting, 0);
    atomic_init(&queue.num_visited, 0);
}

void destroyQueue(void) {
    mtx_destroy(&queue.mutex);
    cnd_destroy(&queue.not_empty);
}

void enqueue(void* item) {
    Node* newNode = malloc(sizeof(Node));
    newNode->data = item;
    newNode->next = NULL;

    mtx_lock(&queue.mutex);

    if (queue.tail == NULL) {
        queue.head = newNode;
        queue.tail = newNode;
    } else {
        queue.tail->next = newNode;
        queue.tail = newNode;
    }

    if (atomic_load(&queue.num_waiting) > 0) {
        cnd_signal(&queue.not_empty);
    }

    mtx_unlock(&queue.mutex);
}

void* dequeue(void) {
    mtx_lock(&queue.mutex);

    while (queue.head == NULL) {
        atomic_fetch_add(&queue.num_waiting, 1);
        cnd_wait(&queue.not_empty, &queue.mutex);
        atomic_fetch_sub(&queue.num_waiting, 1);
    }

    Node* nodeToRemove = queue.head;
    void* item = nodeToRemove->data;
    queue.head = queue.head->next;

    if (queue.head == NULL) {
        queue.tail = NULL;
    }

    atomic_fetch_add(&queue.num_visited, 1);

    mtx_unlock(&queue.mutex);

    return item;
}

bool tryDequeue(void** item) {
    mtx_lock(&queue.mutex);

    if (queue.head == NULL) {
        mtx_unlock(&queue.mutex);
        return false;
    }

    Node* nodeToRemove = queue.head;
    *item = nodeToRemove->data;
    queue.head = queue.head->next;

    if (queue.head == NULL) {
        queue.tail = NULL;
    }

    atomic_fetch_add(&queue.num_visited, 1);

    mtx_unlock(&queue.mutex);

    return true;
}

size_t size(void) {
    size_t count = 0;
    Node* current = queue.head;

    while (current != NULL) {
        count++;
        current = current->next;
    }

    return count;
}

size_t waiting(void) {
    return atomic_load(&queue.num_waiting);
}

size_t visited(void) {
    return atomic_load(&queue.num_visited);
}
