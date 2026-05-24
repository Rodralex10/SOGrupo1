/*
 * types.h — Definições centrais do simulador
 * Constantes, instruções, PCB, algoritmos e plano de chegadas
 */
#ifndef TYPES_H
#define TYPES_H

#include <stddef.h>

/* Limites do simulador */
#define MEMORY_SIZE    1000   /* células do array de instruções */
#define MAX_PROCESSES   50    /* processos simultâneos */
#define MAX_PROG_SLOTS  50    /* programas distintos em memória */
#define MAX_NAME        16    /* nome do .prg (15 chars + '\0') */
#define DEFAULT_QUANTUM  4    /* quantum por defeito */
#define DEFAULT_PRIORITY 5    /* prioridade por defeito no plano */
#define MANAGER_PID      0    /* PID reservado ao gestor */

/* Instruções suportadas nos ficheiros .prg */
typedef enum {
    INS_M = 'M',   /* M n  — variável = n */
    INS_A = 'A',   /* A n  — variável += n */
    INS_S = 'S',   /* S n  — variável -= n */
    INS_B = 'B',   /* B    — bloquear processo */
    INS_T = 'T',   /* T    — terminar processo */
    INS_C = 'C',   /* C n  — fork (filho após C; pai salta n) */
    INS_L = 'L',   /* L f  — carregar programa f */
    INS_NOP = 0    /* célula livre em memória */
} InsOp;

typedef struct {
    InsOp op;
    int   n;                 /* operando numérico (M/A/S/C) */
    char  nome[MAX_NAME];    /* nome do ficheiro (L) */
} Instruction;

/* Estados possíveis de um processo */
typedef enum {
    PROC_NEW,        /* criado, ainda não na fila de prontos */
    PROC_READY,      /* à espera de CPU */
    PROC_RUNNING,    /* a executar */
    PROC_BLOCKED,    /* bloqueado (B ou comando I) */
    PROC_TERMINATED, /* terminou (T ou fim do programa) */
    PROC_WAIT_MEM    /* à espera de memória ou partição */
} ProcState;

/* Bloco de Controlo de Processo */
typedef struct {
    int       pid;
    int       ppid;
    char      program_name[MAX_NAME];
    int       memory_start;   /* índice em memory[] onde começa o código */
    int       memory_len;     /* nº de instruções do programa */
    int       memory_size;    /* tamanho lógico (1.ª linha do .prg, opcional) */
    int       priority;       /* menor valor = mais prioritário */
    int       pc;             /* offset da instrução actual (relativo a memory_start) */
    int       var;            /* variável simulada do processo */
    int       arrival_time;   /* instante de chegada */
    int       start_time;     /* 1.ª vez em RUNNING; -1 se nunca correu */
    int       finish_time;    /* instante de término */
    int       cpu_time_used;  /* ticks de CPU consumidos */
    int       remaining;      /* instruções por executar (usado no SJF) */
    int       period;         /* período para RM (tempo real) */
    int       deadline;       /* prazo absoluto para EDF */
    int       next_release;
    ProcState state;
    int       active;         /* 1 = entrada da tabela em uso */
} PCB;

/* Algoritmos de escalonamento disponíveis */
typedef enum {
    SCHED_FCFS,      /* First-Come First-Served (FIFO) */
    SCHED_PRIORITY,  /* prioridade estática */
    SCHED_SJF,       /* Shortest Job First */
    SCHED_RM,        /* Rate Monotonic */
    SCHED_EDF        /* Earliest Deadline First */
} SchedAlgo;

/* Parâmetros globais da simulação */
typedef struct {
    SchedAlgo algo;
    int       time_quantum;
    int       preemptive;       /* 1 = permite preempção */
    int       memory_total;
    int       partition_count;  /* 0 = sem partições fixas */
    char      plan_file[128];
    char      control_file[128];
} SimConfig;

/* Uma linha do ficheiro plan.txt */
typedef struct {
    char program_name[MAX_NAME];
    int  arrival_time;
    int  priority;
    int  period;
    int  deadline;
    int  launched;   /* 1 = processo já foi criado */
} PlanEntry;

#define MAX_PLAN_ENTRIES 200

#endif /* TYPES_H */
