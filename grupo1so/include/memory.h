/*
 * memory.h — Memória de instruções e partições opcionais
 */
#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"

/* Registo de um programa carregado em memory[] */
typedef struct {
    char name[MAX_NAME];
    int  start;       /* índice inicial em memory[] */
    int  length;      /* nº de instruções */
    int  mem_size;    /* tamanho lógico pedido no .prg */
    int  ref_count;   /* processos que partilham este código */
    int  in_use;
} ProgSlot;

#define MAX_PARTITIONS 32

/* Partição fixa de memória (gestão opcional) */
typedef struct {
    int size;
    int owner_pid;    /* -1 = partição livre */
} Partition;

extern Partition partitions[MAX_PARTITIONS];
extern int       partition_count;

extern Instruction memory[MEMORY_SIZE];
extern ProgSlot    prog_slots[MAX_PROG_SLOTS];
extern int         prog_slot_count;

void memory_init(void);
int  memory_find_slot(const char *name);
int  memory_load_program(const char *name, PCB *pcb_table, int pcb_count);
void memory_release(const char *name, PCB *pcb_table, int pcb_count);
void memory_compact(PCB *pcb_table, int pcb_count);
void memory_dump(void);

void partitions_init(int total_mem, int n_parts);
int  partition_alloc(int pid, int size);
void partition_release(int pid);
int  partitions_enabled(void);

#endif /* MEMORY_H */