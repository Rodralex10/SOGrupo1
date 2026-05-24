#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"

/* ------------------------------------------------------------------ */
/*  Program slot in the memory table                                   */
/* ------------------------------------------------------------------ */
typedef struct {
    char name[MAX_NAME];
    int  start;       /* index into memory[] */
    int  length;      /* number of instructions */
    int  mem_size;    /* logical size from first .prg line (0 = not set) */
    int  ref_count;   /* processes using this program */
    int  in_use;
} ProgSlot;

/* ------------------------------------------------------------------ */
/*  Optional: fixed memory partitions                                  */
/* ------------------------------------------------------------------ */
#define MAX_PARTITIONS 32

typedef struct {
    int size;         /* capacity in abstract memory units */
    int owner_pid;    /* -1 = free */
} Partition;

extern Partition partitions[MAX_PARTITIONS];
extern int       partition_count;

/* ------------------------------------------------------------------ */
/*  Global memory array and slot table                                 */
/* ------------------------------------------------------------------ */
extern Instruction memory[MEMORY_SIZE];
extern ProgSlot    prog_slots[MAX_PROG_SLOTS];
extern int         prog_slot_count;

/* ------------------------------------------------------------------ */
/*  API                                                                 */
/* ------------------------------------------------------------------ */
void memory_init(void);

/* Find the slot index for a program name, -1 if not loaded */
int  memory_find_slot(const char *name);

/*
 * Ensure program is loaded into memory[].
 * Returns the start index, or -1 if memory is full even after compaction.
 * pcb_table/pcb_count are needed for compaction to patch start addresses.
 */
int  memory_load_program(const char *name, PCB *pcb_table, int pcb_count);

/* Decrement ref count; free slot if it reaches zero */
void memory_release(const char *name, PCB *pcb_table, int pcb_count);

/* Compact memory: slide all live segments to the front, patch PCBs */
void memory_compact(PCB *pcb_table, int pcb_count);

/* Print memory map to stdout */
void memory_dump(void);

/* ------------------------------------------------------------------ */
/*  Partition API (optional memory management)                         */
/* ------------------------------------------------------------------ */

/* Initialise partitions with equal size: total_mem / n_parts each */
void partitions_init(int total_mem, int n_parts);

/* Allocate a partition for pid that needs at least 'size' units.
   Returns partition index, or -1 if none available. */
int  partition_alloc(int pid, int size);

/* Release the partition held by pid */
void partition_release(int pid);

/* Returns 1 if partitions are enabled (count > 0), 0 otherwise */
int  partitions_enabled(void);

#endif /* MEMORY_H */
