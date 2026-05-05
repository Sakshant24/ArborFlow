#include "../include/concurrent_q.h"
#include <string.h>

/*
 * concurrent_q.c — Lock-Free SPSC Queue Implementation
 * -----------------------------------------------------
 * Uses atomic operations for thread safety without locks.
 * Assumes single producer and single consumer.
 */

 // Returns concurrent queue struct
ConcurrentQueue *cq_create(void) {
    ConcurrentQueue *q = (ConcurrentQueue *)malloc(sizeof(ConcurrentQueue));
    if (!q) return NULL;

    memset(q, 0, sizeof(ConcurrentQueue));
    // atomic init is used to initialize the atomic variables head and tail to 0. 
    // This ensures that both indices start at the beginning of the buffer. 
    atomic_init(&q->head, 0); 
    atomic_init(&q->tail, 0);

    return q;
}

// frees the memory allocated for the concurrent queue struct
void cq_destroy(ConcurrentQueue *q) {
    if (q) free(q);
}


int cq_enqueue(ConcurrentQueue *q, Packet p) {
    // to load atomic value we use atomic load
    // we use unsigned int for head and tail for future proofing, if in future we use a larger queue size, it should not overflow
    // but for now its 1024, so unsigned int doesn't matter
    unsigned int tail = atomic_load(&q->tail);
    unsigned int next_tail = (tail + 1) % Q_SIZE;

    /* Check if queue is full */
    if (next_tail == atomic_load(&q->head)) {
        return 0;  /* Full */
    }

    /* Store packet */
    q->buffer[tail] = p;

    /* Update tail with atomic_store, as tail is atomic int */
    atomic_store(&q->tail, next_tail);
    return 1;
}

int cq_dequeue(ConcurrentQueue *q, Packet *p) {
    unsigned int head = atomic_load(&q->head);

    /* Check if queue is empty */
    if (head == atomic_load(&q->tail)) {
        return 0;  /* Empty */
    }

    /* Retrieve packet */
    *p = q->buffer[head];

    /* Update head atomically */
    atomic_store(&q->head, (head + 1) % Q_SIZE);
    return 1;
}

int cq_empty(ConcurrentQueue *q) {
    return atomic_load(&q->head) == atomic_load(&q->tail);
}

int cq_full(ConcurrentQueue *q) {
    unsigned int tail = atomic_load(&q->tail);
    unsigned int next_tail = (tail + 1) % Q_SIZE;
    return next_tail == atomic_load(&q->head);
}