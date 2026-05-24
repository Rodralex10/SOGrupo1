#ifndef PROCESS_H
#define PROCESS_H

#include "types.h"

/*
 * Execute one instruction for the process at pcb_table[idx].
 * Returns:
 *   0  - instruction completed normally (advance PC)
 *   1  - process blocked (B)
 *   2  - process terminated (T)
 *   3  - process forked (C)  - new child index in *child_idx_out
 *   4  - process loaded new program (L)
 *
 * new_pid is the next available PID for fork.
 */
int process_exec_one(int idx, PCB *pcb_table, int *pcb_count,
                     int *child_idx_out, int new_pid, int sim_time);

/* Allocate a new PCB slot; returns index or -1 if full */
int process_alloc_pcb(PCB *pcb_table, int *pcb_count);

/* Create a child PCB as copy of parent (C instruction) */
int process_fork(int parent_idx, PCB *pcb_table, int *pcb_count,
                 int new_pid, int sim_time);

#endif /* PROCESS_H */
