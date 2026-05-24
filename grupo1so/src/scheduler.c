/*
 * scheduler.c — Escalonamento de curto e longo prazo
 */
#include "scheduler.h"

/* --- Comparadores para o min-heap --- */

int cmp_priority(int a, int b, const PCB *tbl)
{
    /* Número menor de prioridade = mais urgente; empate por chegada */
    int diff = tbl[a].priority - tbl[b].priority;
    if (diff != 0) return diff;
    return tbl[a].arrival_time - tbl[b].arrival_time;
}

int cmp_sjf(int a, int b, const PCB *tbl)
{
    /* Menos instruções restantes = mais urgente */
    int diff = tbl[a].remaining - tbl[b].remaining;
    if (diff != 0) return diff;
    return tbl[a].arrival_time - tbl[b].arrival_time;
}

int cmp_rm(int a, int b, const PCB *tbl)
{
    /* Rate Monotonic: período mais curto = mais prioritário */
    int da = (tbl[a].period > 0) ? tbl[a].period : 0x7fffffff;
    int db = (tbl[b].period > 0) ? tbl[b].period : 0x7fffffff;
    int diff = da - db;
    if (diff != 0) return diff;
    return tbl[a].arrival_time - tbl[b].arrival_time;
}

int cmp_edf(int a, int b, const PCB *tbl)
{
    /* Earliest Deadline First: prazo mais cedo primeiro */
    int da = (tbl[a].deadline > 0) ? tbl[a].deadline : 0x7fffffff;
    int db = (tbl[b].deadline > 0) ? tbl[b].deadline : 0x7fffffff;
    int diff = da - db;
    if (diff != 0) return diff;
    return tbl[a].arrival_time - tbl[b].arrival_time;
}

/* Coloca processo na fila correcta consoante o algoritmo */
void sched_enqueue(SchedAlgo algo, int idx,
                   FifoQueue *fifo_ready, PrioQueue *prio_ready)
{
    switch (algo) {
    case SCHED_FCFS:
        fifo_enqueue(fifo_ready, idx);
        break;
    case SCHED_PRIORITY:
    case SCHED_SJF:
    case SCHED_RM:
    case SCHED_EDF:
        prio_push(prio_ready, idx);
        break;
    }
}

void sched_remove(SchedAlgo algo, int idx,
                  FifoQueue *fifo_ready, PrioQueue *prio_ready)
{
    switch (algo) {
    case SCHED_FCFS:
        fifo_remove(fifo_ready, idx);
        break;
    case SCHED_PRIORITY:
    case SCHED_SJF:
    case SCHED_RM:
    case SCHED_EDF:
        prio_remove(prio_ready, idx);
        break;
    }
}

/* Escalonador de curto prazo: escolhe o próximo da fila de prontos */
int sched_short_select(SchedAlgo algo,
                       FifoQueue *fifo_ready,
                       PrioQueue *prio_ready,
                       PCB *pcb_table,
                       int sim_time)
{
    (void)pcb_table; (void)sim_time;
    switch (algo) {
    case SCHED_FCFS:
        return fifo_dequeue(fifo_ready);
    case SCHED_PRIORITY:
    case SCHED_SJF:
    case SCHED_RM:
    case SCHED_EDF:
        return prio_pop(prio_ready);
    }
    return -1;
}

/* Verifica se o processo que chegou deve preemptar o que está a correr */
int sched_should_preempt(SchedAlgo algo, int running_idx, int candidate_idx,
                         const PCB *pcb_table, int preemptive, int sim_time)
{
    if (!preemptive) return 0;
    if (running_idx < 0) return 0;

    (void)sim_time;

    switch (algo) {
    case SCHED_FCFS:
        return 0;   /* FCFS não preempta por chegada */

    case SCHED_PRIORITY:
        return cmp_priority(candidate_idx, running_idx, pcb_table) < 0;

    case SCHED_SJF:
        return cmp_sjf(candidate_idx, running_idx, pcb_table) < 0;

    case SCHED_RM:
        return cmp_rm(candidate_idx, running_idx, pcb_table) < 0;

    case SCHED_EDF:
        return cmp_edf(candidate_idx, running_idx, pcb_table) < 0;
    }
    return 0;
}

/*
 * Escalonador de longo prazo (comando D):
 * escolhe aleatoriamente um bloqueado e passa-o para PRONTO.
 */
int sched_long(FifoQueue *blocked_queue,
               FifoQueue *fifo_ready, PrioQueue *prio_ready,
               PCB *pcb_table, SchedAlgo algo)
{
    int n = fifo_size(blocked_queue);
    if (n == 0) return 0;

    int pick_pos = rand() % n;
    int idx = -1;

    for (int i = 0; i < n; i++) {
        int tmp = fifo_dequeue(blocked_queue);
        if (i == pick_pos) {
            idx = tmp;
        } else {
            fifo_enqueue(blocked_queue, tmp);
        }
    }

    if (idx < 0) return 0;

    pcb_table[idx].state = PROC_READY;
    sched_enqueue(algo, idx, fifo_ready, prio_ready);
    printf("[sched_long] Unblocked PID %d\n", pcb_table[idx].pid);
    return 1;
}