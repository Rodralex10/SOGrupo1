/*
 * manager.h — Gestor central da simulação
 * Expõe o estado global e as funções do ciclo de vida
 */
#ifndef MANAGER_H
#define MANAGER_H

#include <strings.h>
#include "types.h"
#include "queues.h"

/* --- Estado global (partilhado por todos os módulos) --- */
extern PCB        pcb_table[MAX_PROCESSES];
extern int        pcb_count;      /* nº de entradas usadas na tabela */
extern int        next_pid;       /* próximo PID a atribuir */
extern int        sim_time;       /* relógio da simulação */
extern int        running_idx;    /* índice do PCB em execução; -1 = CPU livre */
extern SimConfig  cfg;

extern FifoQueue  ready_fifo;       /* fila de prontos (FCFS) */
extern FifoQueue  blocked_queue;    /* processos bloqueados */
extern FifoQueue  terminated_list;  /* processos terminados */

#include "scheduler.h"
extern PrioQueue  ready_prio;       /* fila com prioridade (Priority/SJF/RM/EDF) */

extern PlanEntry  plan_entries[MAX_PLAN_ENTRIES];
extern int        plan_count;

void manager_init(int argc, char **argv);       /* arranque e configuração */
void manager_load_plan(void);                   /* lê plan.txt */
void manager_check_arrivals(void);              /* cria processos no instante certo */
void manager_run(void);                         /* ciclo de comandos E/I/D/R/T */
void manager_exec_quantum(void);                /* comando E */
void manager_interrupt(void);                   /* comando I */
void manager_terminate(void);                   /* comando T — estatísticas */
void manager_make_ready(int idx);               /* coloca na fila de prontos */
void manager_context_switch(void);              /* escolhe próximo processo */

#endif /* MANAGER_H */