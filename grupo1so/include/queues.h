/*
 * queues.h — Filas de índices de PCB
 * FIFO para FCFS; min-heap para algoritmos com prioridade
 */
#ifndef QUEUES_H
#define QUEUES_H

#include "types.h"

/* Fila circular FIFO de índices na tabela de PCB */
typedef struct {
    int data[MAX_PROCESSES];
    int head;    /* primeiro elemento */
    int tail;    /* próxima posição de inserção */
    int count;
} FifoQueue;

void fifo_init(FifoQueue *q);
int  fifo_enqueue(FifoQueue *q, int idx);
int  fifo_dequeue(FifoQueue *q);
int  fifo_peek(const FifoQueue *q);
int  fifo_remove(FifoQueue *q, int idx);
int  fifo_size(const FifoQueue *q);
void fifo_print(const FifoQueue *q, const char *label);

/* Comparador: devolve <0 se a tem mais prioridade que b */
typedef int (*PQCmp)(int idx_a, int idx_b, const PCB *tbl);

typedef struct {
    int    data[MAX_PROCESSES];
    int    count;
    PQCmp  cmp;
    const PCB *tbl;
} PrioQueue;

void prio_init(PrioQueue *q, PQCmp cmp, const PCB *tbl);
int  prio_push(PrioQueue *q, int idx);
int  prio_pop(PrioQueue *q);
int  prio_peek(const PrioQueue *q);
int  prio_remove(PrioQueue *q, int idx);
int  prio_size(const PrioQueue *q);
void prio_rebuild(PrioQueue *q);   /* reconstrói heap após alteração externa */

#endif /* QUEUES_H */
