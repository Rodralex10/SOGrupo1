#ifndef QUEUES_H
#define QUEUES_H

#include "types.h"

/* ------------------------------------------------------------------ */
/*  Simple fixed-size FIFO queue of PCB indices                        */
/* ------------------------------------------------------------------ */
typedef struct {
    int data[MAX_PROCESSES];
    int head;
    int tail;
    int count;
} FifoQueue;

void fifo_init(FifoQueue *q);
int  fifo_enqueue(FifoQueue *q, int idx);  /* returns 0 on success */
int  fifo_dequeue(FifoQueue *q);           /* returns idx or -1 if empty */
int  fifo_peek(const FifoQueue *q);        /* peek front, -1 if empty */
int  fifo_remove(FifoQueue *q, int idx);   /* remove specific idx, 0=ok */
int  fifo_size(const FifoQueue *q);
void fifo_print(const FifoQueue *q, const char *label);

/* ------------------------------------------------------------------ */
/*  Priority queue (min-heap) of PCB indices                           */
/* Uses a comparator callback: returns <0 if a<b, >0 if a>b           */
/* ------------------------------------------------------------------ */
typedef int (*PQCmp)(int idx_a, int idx_b, const PCB *tbl);

typedef struct {
    int    data[MAX_PROCESSES];
    int    count;
    PQCmp  cmp;
    const PCB *tbl;
} PrioQueue;

void prio_init(PrioQueue *q, PQCmp cmp, const PCB *tbl);
int  prio_push(PrioQueue *q, int idx);
int  prio_pop(PrioQueue *q);              /* returns idx or -1 */
int  prio_peek(const PrioQueue *q);
int  prio_remove(PrioQueue *q, int idx);
int  prio_size(const PrioQueue *q);
void prio_rebuild(PrioQueue *q);          /* re-heapify after external update */

#endif /* QUEUES_H */
