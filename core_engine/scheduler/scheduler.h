#ifndef SCHEDULER_H
#define SCHEDULER_H
#include "packet.h"
#define MAX_HEAP 10000
typedef struct {
    Packet heap[MAX_HEAP];
    int size;
} Scheduler;

void scheduler_init(Scheduler *s);

int scheduler_insert(Scheduler *s, Packet p);

int scheduler_extract(Scheduler *s, Packet *p);

Packet* scheduler_peek(Scheduler *s);

int scheduler_empty(Scheduler *s);

#endif