/*
 * process.h — Criação de processos e execução de instruções
 */
#ifndef PROCESS_H
#define PROCESS_H

#include "types.h"

/*
 * Executa uma instrução do processo idx.
 * Retorno: 0=normal | 1=bloqueado | 2=terminado | 3=fork | 4=novo programa
 */
int process_exec_one(int idx, PCB *pcb_table, int *pcb_count,
                     int *child_idx_out, int new_pid, int sim_time);

int process_alloc_pcb(PCB *pcb_table, int *pcb_count);
int process_fork(int parent_idx, PCB *pcb_table, int *pcb_count,
                 int new_pid, int sim_time);

#endif /* PROCESS_H */