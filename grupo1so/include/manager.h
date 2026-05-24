#ifndef MANAGER_H
#define MANAGER_H

#include "types.h"
#include "queues.h"

/* ------------------------------------------------------------------ */
/*  Global simulator state (accessible to all modules via manager.h)  */
/* ------------------------------------------------------------------ */
extern PCB        pcb_table[MAX_PROCESSES];
extern int        pcb_count;
extern int        next_pid;
extern int        sim_time;
extern int        running_idx;   /* -1 = idle */
extern SimConfig  cfg;

extern FifoQueue  ready_fifo;
extern FifoQueue  blocked_queue;
extern FifoQueue  terminated_list;

/* prio_ready is used for Priority / SJF / RM / EDF */
#include "scheduler.h"
extern PrioQueue  ready_prio;

extern PlanEntry  plan_entries[MAX_PLAN_ENTRIES];
extern int        plan_count;

/* ------------------------------------------------------------------ */
/*  API                                                                */
/* ------------------------------------------------------------------ */

/* Parse config file, then override with CLI args */
void manager_init(int argc, char **argv);

/* Load plan.txt into plan_entries */
void manager_load_plan(void);

/* Check plan_entries for arrivals at sim_time, create PCBs */
void manager_check_arrivals(void);

/* Main control loop: read control.txt/stdin and dispatch commands */
void manager_run(void);

/* Execute one quantum for the running process; handle context switch */
void manager_exec_quantum(void);

/* Interrupt and block the running process */
void manager_interrupt(void);

/* Terminate the simulation and print global stats */
void manager_terminate(void);

/* Place a process into the ready structure (handles preemption check) */
void manager_make_ready(int idx);

/* Context switch: save running, pick next, load it */
void manager_context_switch(void);

#endif /* MANAGER_H */
