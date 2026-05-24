/*
 * report.c — Relatório de estado (R) e estatísticas finais (T)
 */
#include "report.h"
#include "memory.h"

/* Nome legível do estado do processo */
static const char *state_name(ProcState s)
{
    switch (s) {
    case PROC_NEW:       return "NOVO";
    case PROC_READY:     return "PRONTO";
    case PROC_RUNNING:   return "EM EXECUCAO";
    case PROC_BLOCKED:   return "BLOQUEADO";
    case PROC_TERMINATED:return "TERMINADO";
    case PROC_WAIT_MEM:  return "AGUARDA MEM";
    default:             return "?";
    }
}

/* Uma linha com os campos principais do PCB */
static void print_pcb_line(const PCB *p)
{
    printf("  PID=%-3d PPID=%-3d PRIO=%-2d VAR=%-6d PROG=%-15s "
           "PC=%-4d CPU=%-4d ARR=%-4d",
           p->pid, p->ppid, p->priority, p->var,
           p->program_name, p->pc, p->cpu_time_used, p->arrival_time);
    if (p->start_time >= 0) printf(" START=%-4d", p->start_time);
    if (p->state == PROC_TERMINATED) printf(" FIN=%-4d", p->finish_time);
    printf("\n");
}

/* Comando R: instantâneo de todas as filas e mapa de memória */
void report_snapshot(int sim_time, int running_idx,
                     const PCB *pcb_table, int pcb_count,
                     const FifoQueue *blocked_queue,
                     const FifoQueue *ready_fifo,
                     const FifoQueue *terminated_list,
                     const FifoQueue *wait_mem_queue)
{
    printf("\n============================\n");
    printf("TEMPO ACTUAL: %d\n", sim_time);
    printf("============================\n");

    printf("PROCESSO EM EXECUCAO:\n");
    if (running_idx >= 0 && pcb_table[running_idx].active) {
        print_pcb_line(&pcb_table[running_idx]);
    } else {
        printf("  (nenhum)\n");
    }

    printf("PROCESSOS PRONTOS A EXECUTAR: [%d]\n", ready_fifo->count);
    for (int i = 0; i < ready_fifo->count; i++) {
        int idx = ready_fifo->data[(ready_fifo->head + i) % MAX_PROCESSES];
        if (idx >= 0 && idx < pcb_count && pcb_table[idx].active)
            print_pcb_line(&pcb_table[idx]);
    }

    printf("PROCESSOS BLOQUEADOS: [%d]\n", blocked_queue->count);
    for (int i = 0; i < blocked_queue->count; i++) {
        int idx = blocked_queue->data[(blocked_queue->head + i) % MAX_PROCESSES];
        if (idx >= 0 && idx < pcb_count && pcb_table[idx].active)
            print_pcb_line(&pcb_table[idx]);
    }

    if (wait_mem_queue && wait_mem_queue->count > 0) {
        printf("PROCESSOS AGUARDANDO MEMORIA: [%d]\n", wait_mem_queue->count);
        for (int i = 0; i < wait_mem_queue->count; i++) {
            int idx = wait_mem_queue->data[(wait_mem_queue->head + i) % MAX_PROCESSES];
            if (idx >= 0 && idx < pcb_count && pcb_table[idx].active)
                print_pcb_line(&pcb_table[idx]);
        }
    }

    printf("PROCESSOS TERMINADOS: [%d]\n", terminated_list->count);
    for (int i = 0; i < terminated_list->count; i++) {
        int idx = terminated_list->data[(terminated_list->head + i) % MAX_PROCESSES];
        if (idx >= 0 && idx < pcb_count && pcb_table[idx].active)
            print_pcb_line(&pcb_table[idx]);
    }

    memory_dump();
    printf("============================\n\n");
}

/* Comando T: turnaround, espera e utilização da CPU */
void report_global_stats(int sim_time,
                         const PCB *pcb_table, int pcb_count)
{
    printf("\n======== ESTATISTICAS GLOBAIS ========\n");
    printf("Tempo total de simulacao: %d\n", sim_time);

    int total = 0, finished = 0;
    double sum_turnaround = 0.0, sum_wait = 0.0;
    int total_cpu = 0;

    for (int i = 0; i < pcb_count; i++) {
        const PCB *p = &pcb_table[i];
        if (!p->active) continue;
        if (p->pid == MANAGER_PID) continue;   /* ignorar o gestor */

        total++;
        total_cpu += p->cpu_time_used;

        if (p->state == PROC_TERMINATED) {
            finished++;
            int turnaround = p->finish_time - p->arrival_time;
            int wait       = turnaround - p->cpu_time_used;
            sum_turnaround += turnaround;
            sum_wait       += wait;
            printf("  PID=%-3d PRIO=%-2d ARR=%-4d FIN=%-4d TURNAROUND=%-4d WAIT=%-4d CPU=%-4d\n",
                   p->pid, p->priority, p->arrival_time, p->finish_time,
                   turnaround, wait, p->cpu_time_used);
        } else {
            printf("  PID=%-3d PRIO=%-2d ARR=%-4d estado=%-12s CPU=%-4d\n",
                   p->pid, p->priority, p->arrival_time,
                   state_name(p->state), p->cpu_time_used);
        }
    }

    if (finished > 0) {
        printf("Turnaround medio: %.2f\n", sum_turnaround / finished);
        printf("Espera media:     %.2f\n", sum_wait       / finished);
    }
    if (sim_time > 0)
        printf("Utilizacao CPU:   %.1f%%\n",
               100.0 * total_cpu / sim_time);
    printf("Processos criados:    %d\n", total);
    printf("Processos terminados: %d\n", finished);
    printf("======================================\n");
}
