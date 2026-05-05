#ifndef CONCURRENT_Q_H
#define CONCURRENT_Q_H

/*
 Thread-safe queue for passing packets between capture thread
 and processing threads without locks (using atomic operations).

 Implementation: Single-producer, single-consumer (SPSC) lock-free queue.
 For simplicity, we use a circular buffer with atomic indices.
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdatomic.h>
#include "../scheduler/packet.h"

#define Q_SIZE 1024  /* Must be power of 2 for efficient modulo */

/*
Atomic ints are used for head and tail indices to ensure thread safety without locks.
For example in an normal int, adding a int like xyz = xyz + 5 its actually 3 process 
1. read value of xyz
2. add 1 to it
3. write back to xyz
Now if two threads do operation of xyz++ at the same time, they might both read the same value,
and write back the same value, resulting in loss of one increment. But with atomic ints, the operations are indivisible,
so one thread will complete its increment before the other thread can read the value, ensuring correct behavior.
*/
typedef struct {
    Packet buffer[Q_SIZE];
    atomic_uint head;  /* Consumer reads from head */
    atomic_uint tail;  /* Producer writes to tail */
} ConcurrentQueue;

/*
 cq_create — Allocate and initialize queue.
 */
ConcurrentQueue *cq_create(void);

/*
 cq_destroy — Free queue memory.
 */
void cq_destroy(ConcurrentQueue *q);

/*
 cq_enqueue — Add packet to queue (producer side).
 Returns 1 if successful, 0 if queue full.
 */
int cq_enqueue(ConcurrentQueue *q, Packet p);

/*
 cq_dequeue — Remove packet from queue (consumer side).
 Returns 1 if successful, 0 if queue empty.
 */
int cq_dequeue(ConcurrentQueue *q, Packet *p);

/*
 cq_empty — Check if queue is empty.
 */
int cq_empty(ConcurrentQueue *q);

/*
 cq_full — Check if queue is full.
 */
int cq_full(ConcurrentQueue *q);

#endif