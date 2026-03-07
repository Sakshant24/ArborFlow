#include<stdio.h>
#include "scheduler.h" //max heap 
static void swap(Packet *a, Packet *b){
    Packet temp = *a;
    *a =  *b;
    *b = temp;
}
void scheduler_init(Scheduler *s){
    s->size=0;
}
static void heapify_up(Scheduler *s, int index){
    while(index > 0)
    {
        int parent = (index - 1) / 2;
        if(s->heap[index].priority > s->heap[parent].priority)
        {
            swap(&s->heap[index], &s->heap[parent]);
            index = parent;
        }
        else    break;
    }
}
static void heapify_down(Scheduler *s, int index){
    while(1)
    {
        int left = 2*index + 1;
        int right = 2*index + 2;
        int largest = index;
        if(left < s->size && s->heap[left].priority > s->heap[largest].priority){
            largest = left;
        }
        if(right < s->size && s->heap[right].priority > s->heap[largest].priority){
            largest = right;
        }
        if(largest == index)    break;
        swap(&s->heap[index], &s->heap[largest]);
        index = largest;
    }
}
int scheduler_insert(Scheduler *s, Packet p){
    if(s->size >= MAX_HEAP){
        return -1;
    }
    s->heap[s->size] = p;
    heapify_up(s, s->size);
    s->size++;
    return 0;
}
int scheduler_extract(Scheduler *s, Packet *p){ //max
    if(s->size == 0){
        return -1;
    }
    *p = s->heap[0];
    s->heap[0] = s->heap[s->size - 1];
    s->size--;
    heapify_down(s, 0);
    return 0;
}
Packet* scheduler_peek(Scheduler *s){
    if(s->size == 0){
        return NULL;
    }
    return &s->heap[0];
}

int scheduler_empty(Scheduler *s){
    return s->size == 0;
}
int main(){ //test program
    Scheduler s;
    scheduler_init(&s);
    Packet p1 = {1,2,6,100,10};
    Packet p2 = {1,2,6,100,5};
    Packet p3 = {1,2,6,100,8};
    scheduler_insert(&s,p1);
    scheduler_insert(&s,p2);
    scheduler_insert(&s,p3);
    Packet p;
    while(!scheduler_empty(&s)){
        scheduler_extract(&s,&p);
        printf("Priority served: %d\n",p.priority);
    }
}
