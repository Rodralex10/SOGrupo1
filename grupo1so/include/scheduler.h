/*
 * scheduler.h — Escalonamento de curto e longo prazo
 */
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "types.h"
#include "queues.h"

int sched_short_select(SchedAlgo algo,
                       FifoQueue *fifo_ready,
                       PrioQueue *prio_ready,
                       PCB *pcb_table,
                       int sim_time);

int sched_should_preempt(SchedAlgo algo, int running_idx, int candidate_idx,
                         const PCB *pcb_table, int preemptive, int sim_time);

void sched_enqueue(SchedAlgo algo, int idx,
                   FifoQueue *fifo_ready, PrioQueue *prio_ready);

void sched_remove(SchedAlgo algo, int idx,
                  FifoQueue *fifo_ready, PrioQueue *prio_ready);

int sched_long(FifoQueue *blocked_queue,
               FifoQueue *fifo_ready, PrioQueue *prio_ready,
               PCB *pcb_table, SchedAlgo algo);

int cmp_priority(int a, int b, const PCB *tbl);
int cmp_sjf(int a, int b, const PCB *tbl);
int cmp_rm(int a, int b, const PCB *tbl);
int cmp_edf(int a, int b, const PCB *tbl);

#endif /* SCHEDULER_H */
