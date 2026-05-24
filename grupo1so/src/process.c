/*
 * process.c — PCB, fork e interpretador de instruções
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "process.h"
#include "memory.h"
#include "loader.h"

/* Procura primeira entrada livre na tabela de PCB */
int process_alloc_pcb(PCB *pcb_table, int *pcb_count)
{
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (!pcb_table[i].active) {
            memset(&pcb_table[i], 0, sizeof(PCB));
            pcb_table[i].active     = 1;
            pcb_table[i].start_time = -1;
            if (i >= *pcb_count) *pcb_count = i + 1;
            return i;
        }
    }
    fprintf(stderr, "process: PCB table full (max %d)\n", MAX_PROCESSES);
    return -1;
}

/* Cria filho como cópia do pai (instrução C) */
int process_fork(int parent_idx, PCB *pcb_table, int *pcb_count,
                 int new_pid, int sim_time)
{
    int ci = process_alloc_pcb(pcb_table, pcb_count);
    if (ci < 0) return -1;

    PCB *parent = &pcb_table[parent_idx];
    PCB *child  = &pcb_table[ci];

    *child = *parent;
    child->pid          = new_pid;
    child->ppid         = parent->pid;
    child->state        = PROC_READY;
    child->cpu_time_used = 0;
    child->start_time   = -1;
    child->finish_time  = 0;
    child->arrival_time = sim_time;

    /* Programa partilhado: mais um processo usa o mesmo código */
    int slot = memory_find_slot(parent->program_name);
    if (slot >= 0) prog_slots[slot].ref_count++;

    return ci;
}

int process_exec_one(int idx, PCB *pcb_table, int *pcb_count,
                     int *child_idx_out, int new_pid, int sim_time)
{
    PCB *p = &pcb_table[idx];
    int abs_pc = p->memory_start + p->pc;   /* PC absoluto em memory[] */

    if (abs_pc < 0 || abs_pc >= MEMORY_SIZE) {
        fprintf(stderr, "process %d: PC %d out of range\n", p->pid, abs_pc);
        p->state       = PROC_TERMINATED;
        p->finish_time = sim_time + 1;
        return 2;
    }

    Instruction ins = memory[abs_pc];

    if (ins.op == INS_NOP) {
        /* Fim do programa sem T explícito */
        p->state       = PROC_TERMINATED;
        p->finish_time = sim_time + 1;
        return 2;
    }

    p->cpu_time_used++;
    p->remaining = (p->remaining > 0) ? p->remaining - 1 : 0;

    switch (ins.op) {
    case INS_M:   /* atribuir */
        p->var = ins.n;
        p->pc++;
        return 0;

    case INS_A:   /* somar */
        p->var += ins.n;
        p->pc++;
        return 0;

    case INS_S:   /* subtrair */
        p->var -= ins.n;
        p->pc++;
        return 0;

    case INS_B:   /* bloquear */
        p->state = PROC_BLOCKED;
        p->pc++;
        return 1;

    case INS_T:   /* terminar e libertar memória */
        p->state       = PROC_TERMINATED;
        p->finish_time = sim_time + 1;
        memory_release(p->program_name, pcb_table, *pcb_count);
        return 2;

    case INS_C: {
        /*
         * Fork: filho executa a instrução logo após C;
         * pai salta para pc + 1 + n (ignora n instruções).
         */
        int child_pc = p->pc + 1;
        p->pc = p->pc + 1 + ins.n;

        int ci = process_fork(idx, pcb_table, pcb_count, new_pid, sim_time);
        if (ci < 0) return 0;

        pcb_table[ci].pc = child_pc;

        if (child_idx_out) *child_idx_out = ci;
        return 3;
    }

    case INS_L: {
        /* Troca de programa: liberta o antigo e carrega o novo */
        memory_release(p->program_name, pcb_table, *pcb_count);

        int start = memory_load_program(ins.nome, pcb_table, *pcb_count);
        if (start < 0) {
            p->state       = PROC_TERMINATED;
            p->finish_time = sim_time + 1;
            return 2;
        }

        int slot = memory_find_slot(ins.nome);
        p->memory_start = start;
        p->memory_len   = (slot >= 0) ? prog_slots[slot].length : 0;
        p->remaining    = p->memory_len;
        p->pc           = 0;
        strncpy(p->program_name, ins.nome, MAX_NAME - 1);
        return 4;
    }

    default:
        p->pc++;
        return 0;
    }
}
