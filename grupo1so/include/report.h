/*
 * report.h — Relatórios de estado e estatísticas finais
 */
#ifndef REPORT_H
#define REPORT_H

#include "types.h"
#include "queues.h"

void report_snapshot(int sim_time, int running_idx,
                     const PCB *pcb_table, int pcb_count,
                     const FifoQueue *blocked_queue,
                     const FifoQueue *ready_fifo,
                     const FifoQueue *terminated_list,
                     const FifoQueue *wait_mem_queue);

void report_global_stats(int sim_time,
                         const PCB *pcb_table, int pcb_count);

#endif /* REPORT_H */