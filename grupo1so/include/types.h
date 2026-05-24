#ifndef TYPES_H
#define TYPES_H

#include <stddef.h>

/* ------------------------------------------------------------------ */
/*  Constants                                                           */
/* ------------------------------------------------------------------ */
#define MEMORY_SIZE    1000
#define MAX_PROCESSES   50
#define MAX_PROG_SLOTS  50   /* distinct programs in memory */
#define MAX_NAME        16   /* 15 chars + '\0' */
#define DEFAULT_QUANTUM  4
#define DEFAULT_PRIORITY 5
#define MANAGER_PID      0

/* ------------------------------------------------------------------ */
/*  Instruction set                                                     */
/* ------------------------------------------------------------------ */
typedef enum {
    INS_M = 'M',
    INS_A = 'A',
    INS_S = 'S',
    INS_B = 'B',
    INS_T = 'T',
    INS_C = 'C',
    INS_L = 'L',
    INS_NOP = 0
} InsOp;

typedef struct {
    InsOp op;
    int   n;
    char  nome[MAX_NAME];
} Instruction;

/* ------------------------------------------------------------------ */
/*  Process states                                                      */
/* ------------------------------------------------------------------ */
typedef enum {
    PROC_NEW,
    PROC_READY,
    PROC_RUNNING,
    PROC_BLOCKED,
    PROC_TERMINATED,
    PROC_WAIT_MEM
} ProcState;

/* ------------------------------------------------------------------ */
/*  Process Control Block                                              */
/* ------------------------------------------------------------------ */
typedef struct {
    int       pid;
    int       ppid;
    char      program_name[MAX_NAME];
    int       memory_start;   /* absolute index in memory[] */
    int       memory_len;     /* number of instructions in program */
    int       memory_size;    /* logical size from first .prg line (optional) */
    int       priority;
    int       pc;             /* offset from memory_start */
    int       var;
    int       arrival_time;
    int       start_time;     /* first time slice in RUNNING, -1 if not yet */
    int       finish_time;
    int       cpu_time_used;
    int       remaining;      /* SJF: instructions left */
    /* real-time fields */
    int       period;
    int       deadline;       /* relative to period start */
    int       next_release;
    ProcState state;
    int       active;         /* slot is in use */
} PCB;

/* ------------------------------------------------------------------ */
/*  Scheduling algorithms                                              */
/* ------------------------------------------------------------------ */
typedef enum {
    SCHED_FCFS,
    SCHED_PRIORITY,
    SCHED_SJF,
    SCHED_RM,
    SCHED_EDF
} SchedAlgo;

/* ------------------------------------------------------------------ */
/*  Global simulator configuration                                     */
/* ------------------------------------------------------------------ */
typedef struct {
    SchedAlgo algo;
    int       time_quantum;
    int       preemptive;
    int       memory_total;
    int       partition_count;
    char      plan_file[128];
    char      control_file[128];
} SimConfig;

/* ------------------------------------------------------------------ */
/*  Pending arrival entry (from plan.txt)                              */
/* ------------------------------------------------------------------ */
typedef struct {
    char program_name[MAX_NAME];
    int  arrival_time;
    int  priority;
    int  period;
    int  deadline;
    int  launched;
} PlanEntry;

#define MAX_PLAN_ENTRIES 200

#endif /* TYPES_H */
