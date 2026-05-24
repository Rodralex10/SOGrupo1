#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "types.h"
#include "queues.h"

/* ------------------------------------------------------------------ */
/*  Short-term scheduler                                               */
/* ------------------------------------------------------------------ */

/*
 * Select the next process to run from the ready structure.
 * Returns the PCB index of the winner, or -1 if no ready process.
 * For FCFS/SJF the ready structure is a FifoQueue.
 * For Priority/RM/EDF it is a PrioQueue.
 */
int sched_short_select(SchedAlgo algo,
                       FifoQueue *fifo_ready,
                       PrioQueue *prio_ready,
                       PCB *pcb_table,
                       int sim_time);

/*
 * Decide whether to preempt the currently running process in favour of
 * a newly added ready process. Returns 1 if preemption should occur.
 */
int sched_should_preempt(SchedAlgo algo, int running_idx, int candidate_idx,
                         const PCB *pcb_table, int preemptive, int sim_time);

/* Enqueue idx into the right structure based on algo */
void sched_enqueue(SchedAlgo algo, int idx,
                   FifoQueue *fifo_ready, PrioQueue *prio_ready);

/* Remove idx from the right structure */
void sched_remove(SchedAlgo algo, int idx,
                  FifoQueue *fifo_ready, PrioQueue *prio_ready);

/* ------------------------------------------------------------------ */
/*  Long-term scheduler                                                */
/* ------------------------------------------------------------------ */

/*
 * Move one or more processes from blocked_queue to ready.
 * Returns the number of processes unblocked.
 */
int sched_long(FifoQueue *blocked_queue,
               FifoQueue *fifo_ready, PrioQueue *prio_ready,
               PCB *pcb_table, SchedAlgo algo);

/* ------------------------------------------------------------------ */
/*  Comparators for PrioQueue                                          */
/* ------------------------------------------------------------------ */
int cmp_priority(int a, int b, const PCB *tbl);
int cmp_sjf(int a, int b, const PCB *tbl);
int cmp_rm(int a, int b, const PCB *tbl);
int cmp_edf(int a, int b, const PCB *tbl);

#endif /* SCHEDULER_H */
