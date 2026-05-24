/*
 * manager.c — Núcleo do simulador
 * Configuração, chegadas, escalonamento, comandos E/I/D/R/T
 */
#define _POSIX_C_SOURCE 200809L
#include "manager.h"
#include "memory.h"
#include "loader.h"
#include "process.h"
#include "scheduler.h"
#include "report.h"

/* --- Variáveis globais da simulação --- */
PCB        pcb_table[MAX_PROCESSES];
int        pcb_count   = 0;
int        next_pid    = 1;
int        sim_time    = 0;
int        running_idx = -1;
SimConfig  cfg;

FifoQueue  ready_fifo;
FifoQueue  blocked_queue;
FifoQueue  terminated_list;
FifoQueue  wait_mem_queue;   /* à espera de memória ou partição */
PrioQueue  ready_prio;

PlanEntry  plan_entries[MAX_PLAN_ENTRIES];
int        plan_count  = 0;

/* Converte string do config/CLI em algoritmo */
static SchedAlgo parse_algo(const char *s)
{
    if (strcasecmp(s, "priority") == 0) return SCHED_PRIORITY;
    if (strcasecmp(s, "sjf")      == 0) return SCHED_SJF;
    if (strcasecmp(s, "rm")       == 0) return SCHED_RM;
    if (strcasecmp(s, "edf")      == 0) return SCHED_EDF;
    return SCHED_FCFS;
}

/* Escolhe o comparador do heap consoante o algoritmo */
static PQCmp algo_cmp(SchedAlgo algo)
{
    switch (algo) {
    case SCHED_PRIORITY: return cmp_priority;
    case SCHED_SJF:      return cmp_sjf;
    case SCHED_RM:       return cmp_rm;
    case SCHED_EDF:      return cmp_edf;
    default:             return cmp_priority;
    }
}

/* Lê ficheiro chave=valor (linhas # são comentários) */
static void load_config_file(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) return;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '#' || line[0] == '\0') continue;
        char key[64], val[128];
        if (sscanf(line, "%63[^=]=%127s", key, val) != 2) continue;
        char *k = key;
        while (isspace((unsigned char)*k)) k++;
        char *end = k + strlen(k) - 1;
        while (end > k && isspace((unsigned char)*end)) *end-- = '\0';

        if (strcmp(k, "scheduler")       == 0) cfg.algo = parse_algo(val);
        else if (strcmp(k, "quantum")    == 0) cfg.time_quantum = atoi(val);
        else if (strcmp(k, "preemptive") == 0) cfg.preemptive   = atoi(val);
        else if (strcmp(k, "memory_total")     == 0) cfg.memory_total     = atoi(val);
        else if (strcmp(k, "partition_count")  == 0) cfg.partition_count  = atoi(val);
        else if (strcmp(k, "plan_file")        == 0) strncpy(cfg.plan_file, val, 127);
        else if (strcmp(k, "control_file")     == 0) strncpy(cfg.control_file, val, 127);
    }
    fclose(f);
}

void manager_init(int argc, char **argv)
{
    srand((unsigned)time(NULL));   /* para desbloqueio aleatório no D */

    /* Valores por defeito */
    cfg.algo            = SCHED_FCFS;
    cfg.time_quantum    = DEFAULT_QUANTUM;
    cfg.preemptive      = 1;
    cfg.memory_total    = MEMORY_SIZE;
    cfg.partition_count = 4;
    strncpy(cfg.plan_file,    "data/plan.txt",    127);
    strncpy(cfg.control_file, "data/control.txt", 127);

    load_config_file("data/config.txt");
    load_config_file("config.txt");

    /* Opções da linha de comandos têm prioridade sobre o ficheiro */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--scheduler") == 0 && i+1 < argc)
            cfg.algo = parse_algo(argv[++i]);
        else if (strcmp(argv[i], "--quantum") == 0 && i+1 < argc)
            cfg.time_quantum = atoi(argv[++i]);
        else if (strcmp(argv[i], "--plan") == 0 && i+1 < argc)
            strncpy(cfg.plan_file, argv[++i], 127);
        else if (strcmp(argv[i], "--control") == 0 && i+1 < argc)
            strncpy(cfg.control_file, argv[++i], 127);
        else if (strcmp(argv[i], "--no-preempt") == 0)
            cfg.preemptive = 0;
        else if (strcmp(argv[i], "--memory") == 0 && i+1 < argc)
            cfg.memory_total = atoi(argv[++i]);
        else if (strcmp(argv[i], "--partitions") == 0 && i+1 < argc)
            cfg.partition_count = atoi(argv[++i]);
    }

    memory_init();
    partitions_init(cfg.memory_total, cfg.partition_count);
    fifo_init(&ready_fifo);
    fifo_init(&blocked_queue);
    fifo_init(&terminated_list);
    fifo_init(&wait_mem_queue);
    prio_init(&ready_prio, algo_cmp(cfg.algo), pcb_table);

    /* PCB do gestor (PID 0) — não executa instruções de utilizador */
    memset(pcb_table, 0, sizeof(pcb_table));
    pcb_table[0].pid    = MANAGER_PID;
    pcb_table[0].ppid   = -1;
    pcb_table[0].active = 1;
    strncpy(pcb_table[0].program_name, "gestor", MAX_NAME - 1);
    pcb_table[0].state  = PROC_RUNNING;
    pcb_count           = 1;

    manager_load_plan();
    printf("[manager] Simulador iniciado. Algo=%d Quantum=%d Preemptive=%d\n",
           cfg.algo, cfg.time_quantum, cfg.preemptive);
}

void manager_load_plan(void)
{
    FILE *f = fopen(cfg.plan_file, "r");
    if (!f) {
        fprintf(stderr, "manager: cannot open plan file '%s'\n", cfg.plan_file);
        return;
    }
    char line[256];
    while (fgets(line, sizeof(line), f) && plan_count < MAX_PLAN_ENTRIES) {
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '#' || line[0] == '\0') continue;

        PlanEntry e;
        memset(&e, 0, sizeof(e));
        e.priority = DEFAULT_PRIORITY;

        /* Formato: programa.prg  chegada  [prioridade  [periodo  [prazo]]] */
        int n = sscanf(line, "%15s %d %d %d %d",
                       e.program_name, &e.arrival_time,
                       &e.priority, &e.period, &e.deadline);
        if (n >= 2) {
            plan_entries[plan_count++] = e;
        }
    }
    fclose(f);
    printf("[manager] Loaded %d plan entries from '%s'\n",
           plan_count, cfg.plan_file);
}

/*
 * Para cada entrada do plano cuja chegada <= sim_time,
 * cria PCB, carrega programa e coloca em PRONTO (ou WAIT_MEM).
 */
void manager_check_arrivals(void)
{
    for (int i = 0; i < plan_count; i++) {
        PlanEntry *e = &plan_entries[i];
        if (e->launched || e->arrival_time > sim_time) continue;

        int idx = process_alloc_pcb(pcb_table, &pcb_count);
        if (idx < 0) continue;

        PCB *p = &pcb_table[idx];
        strncpy(p->program_name, e->program_name, MAX_NAME - 1);
        p->pid          = next_pid++;
        p->ppid         = MANAGER_PID;
        p->priority     = e->priority;
        p->period       = e->period;
        p->deadline     = e->deadline;
        p->arrival_time = sim_time;
        p->start_time   = -1;
        p->state        = PROC_NEW;

        int start = memory_load_program(e->program_name, pcb_table, pcb_count);
        if (start < 0) {
            p->state = PROC_WAIT_MEM;
            fifo_enqueue(&wait_mem_queue, idx);
            printf("[manager] PID %d '%s' aguardando memoria (t=%d)\n",
                   p->pid, p->program_name, sim_time);
        } else {
            int slot = memory_find_slot(e->program_name);
            p->memory_start = start;
            p->memory_len   = (slot >= 0) ? prog_slots[slot].length : 0;
            p->memory_size  = (slot >= 0) ? prog_slots[slot].mem_size : 0;
            p->remaining    = p->memory_len;
            p->next_release = e->arrival_time;
            if (p->deadline == 0 && p->period > 0)
                p->deadline = p->arrival_time + p->period;

            if (partitions_enabled()) {
                int needed = (p->memory_size > 0) ? p->memory_size : 1;
                int part = partition_alloc(p->pid, needed);
                if (part < 0) {
                    memory_release(p->program_name, pcb_table, pcb_count);
                    p->state = PROC_WAIT_MEM;
                    fifo_enqueue(&wait_mem_queue, idx);
                    printf("[manager] PID %d '%s' aguardando particao (t=%d)\n",
                           p->pid, p->program_name, sim_time);
                    e->launched = 1;
                    continue;
                }
            }

            printf("[manager] PID %d '%s' criado e pronto (t=%d)\n",
                   p->pid, p->program_name, sim_time);
            manager_make_ready(idx);
        }
        e->launched = 1;
    }
}

void manager_make_ready(int idx)
{
    pcb_table[idx].state = PROC_READY;
    sched_enqueue(cfg.algo, idx, &ready_fifo, &ready_prio);

    /* Se o novo processo for mais prioritário, preempta o actual */
    if (running_idx >= 0 &&
        sched_should_preempt(cfg.algo, running_idx, idx,
                             pcb_table, cfg.preemptive, sim_time)) {
        printf("[manager] Preempcao: PID %d desloca PID %d\n",
               pcb_table[idx].pid, pcb_table[running_idx].pid);
        int old = running_idx;
        running_idx = -1;
        pcb_table[old].state = PROC_READY;
        sched_enqueue(cfg.algo, old, &ready_fifo, &ready_prio);
        manager_context_switch();
    }
}

void manager_context_switch(void)
{
    int next = sched_short_select(cfg.algo, &ready_fifo, &ready_prio,
                                  pcb_table, sim_time);
    if (next < 0) {
        running_idx = -1;
        return;
    }
    running_idx = next;
    pcb_table[next].state = PROC_RUNNING;
    if (pcb_table[next].start_time < 0)
        pcb_table[next].start_time = sim_time;
    printf("[ctx] PID %d agora em execucao (t=%d)\n",
           pcb_table[next].pid, sim_time);
}

/*
 * Comando E: executa até time_quantum ticks.
 * Em cada tick: uma instrução, avança relógio, trata bloqueio/termino/fork.
 */
void manager_exec_quantum(void)
{
    manager_check_arrivals();

    if (running_idx < 0) {
        manager_context_switch();
        if (running_idx < 0) {
            printf("[exec] Nenhum processo pronto. t=%d\n", sim_time);
            sim_time++;
            return;
        }
    }

    int ticks_left = cfg.time_quantum;
    while (ticks_left > 0 && running_idx >= 0) {
        int child_idx = -1;
        int ret = process_exec_one(running_idx, pcb_table, &pcb_count,
                                   &child_idx, next_pid, sim_time);
        if (ret == 3) next_pid++;
        sim_time++;
        ticks_left--;

        manager_check_arrivals();

        switch (ret) {
        case 1:   /* instrução B — bloqueado */
            fifo_enqueue(&blocked_queue, running_idx);
            printf("[exec] PID %d bloqueado (t=%d)\n",
                   pcb_table[running_idx].pid, sim_time);
            running_idx = -1;
            manager_context_switch();
            break;

        case 2:   /* instrução T ou fim — terminado */
            printf("[exec] PID %d terminou (t=%d)\n",
                   pcb_table[running_idx].pid, sim_time);
            if (partitions_enabled())
                partition_release(pcb_table[running_idx].pid);
            fifo_enqueue(&terminated_list, running_idx);
            running_idx = -1;
            manager_context_switch();
            break;

        case 3:   /* instrução C — filho criado */
            if (child_idx >= 0) {
                printf("[exec] PID %d criou filho PID %d (t=%d)\n",
                       pcb_table[running_idx].pid,
                       pcb_table[child_idx].pid, sim_time);
                manager_make_ready(child_idx);
            }
            break;

        case 4:   /* instrução L — programa substituído */
            printf("[exec] PID %d carregou novo programa '%s' (t=%d)\n",
                   pcb_table[running_idx].pid,
                   pcb_table[running_idx].program_name, sim_time);
            if (cfg.algo == SCHED_SJF) prio_rebuild(&ready_prio);
            break;

        default:
            break;
        }
    }

    /* Quantum esgotado sem bloquear/terminar: volta à fila de prontos */
    if (running_idx >= 0 && ticks_left == 0) {
        printf("[exec] Quantum expirado para PID %d (t=%d)\n",
               pcb_table[running_idx].pid, sim_time);
        int old = running_idx;
        running_idx = -1;
        pcb_table[old].state = PROC_READY;
        sched_enqueue(cfg.algo, old, &ready_fifo, &ready_prio);
        manager_context_switch();
    }
}

/* Comando I: bloqueia o processo em execução (como um B externo) */
void manager_interrupt(void)
{
    if (running_idx < 0) {
        printf("[interrupt] Nenhum processo em execucao.\n");
        return;
    }
    printf("[interrupt] PID %d interrompido e bloqueado (t=%d)\n",
           pcb_table[running_idx].pid, sim_time);
    pcb_table[running_idx].state = PROC_BLOCKED;
    fifo_enqueue(&blocked_queue, running_idx);
    running_idx = -1;
    manager_context_switch();
}

/* Comando T: imprime estatísticas e termina */
void manager_terminate(void)
{
    report_global_stats(sim_time, pcb_table, pcb_count);
}

/*
 * Ciclo principal: lê control.txt ou stdin.
 * E = executar quantum
 * I = interromper
 * D = desbloquear (longo prazo) + tentar carregar WAIT_MEM
 * R = relatório
 * T = terminar simulação
 */
void manager_run(void)
{
    FILE *ctrl = NULL;
    int use_stdin = 0;

    if (strcmp(cfg.control_file, "-") == 0) {
        use_stdin = 1;
    } else {
        ctrl = fopen(cfg.control_file, "r");
        if (!ctrl) {
            fprintf(stderr,
                    "manager: cannot open control file '%s', using stdin\n",
                    cfg.control_file);
            use_stdin = 1;
        }
    }

    char line[256];
    int running = 1;

    while (running) {
        if (use_stdin) {
            printf("sim> ");
            fflush(stdout);
            if (!fgets(line, sizeof(line), stdin)) break;
        } else {
            if (!fgets(line, sizeof(line), ctrl)) break;
        }

        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '#' || line[0] == '\0') continue;

        char cmd = (char)toupper((unsigned char)line[0]);
        switch (cmd) {
        case 'E':
            manager_exec_quantum();
            break;
        case 'I':
            manager_interrupt();
            break;
        case 'D':
            sched_long(&blocked_queue, &ready_fifo, &ready_prio,
                       pcb_table, cfg.algo);
            /* Re-tentar processos que esperavam memória */
            for (int i = 0; i < fifo_size(&wait_mem_queue); ) {
                int idx = fifo_dequeue(&wait_mem_queue);
                int start = memory_load_program(pcb_table[idx].program_name,
                                                pcb_table, pcb_count);
                if (start >= 0) {
                    int slot = memory_find_slot(pcb_table[idx].program_name);
                    pcb_table[idx].memory_start = start;
                    pcb_table[idx].memory_len =
                        (slot >= 0) ? prog_slots[slot].length : 0;
                    pcb_table[idx].memory_size =
                        (slot >= 0) ? prog_slots[slot].mem_size : 0;
                    pcb_table[idx].remaining = pcb_table[idx].memory_len;
                    if (partitions_enabled()) {
                        int needed = (pcb_table[idx].memory_size > 0)
                                     ? pcb_table[idx].memory_size : 1;
                        int part = partition_alloc(pcb_table[idx].pid, needed);
                        if (part < 0) {
                            memory_release(pcb_table[idx].program_name,
                                           pcb_table, pcb_count);
                            fifo_enqueue(&wait_mem_queue, idx);
                            i++;
                            continue;
                        }
                    }
                    manager_make_ready(idx);
                } else {
                    fifo_enqueue(&wait_mem_queue, idx);
                    i++;
                }
            }
            break;
        case 'R':
            report_snapshot(sim_time, running_idx,
                            pcb_table, pcb_count,
                            &blocked_queue, &ready_fifo,
                            &terminated_list, &wait_mem_queue);
            break;
        case 'T':
            manager_terminate();
            running = 0;
            break;
        default:
            fprintf(stderr, "manager: unknown command '%c'\n", cmd);
            break;
        }
    }

    if (ctrl) fclose(ctrl);

    if (running) manager_terminate();   /* fim do ficheiro sem T */
}