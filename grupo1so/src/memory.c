#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "memory.h"
#include "loader.h"

Instruction memory[MEMORY_SIZE];
ProgSlot    prog_slots[MAX_PROG_SLOTS];
int         prog_slot_count = 0;

Partition   partitions[MAX_PARTITIONS];
int         partition_count = 0;

void memory_init(void)
{
    memset(memory, 0, sizeof(memory));
    memset(prog_slots, 0, sizeof(prog_slots));
    prog_slot_count = 0;
    memset(partitions, 0, sizeof(partitions));
    for (int i = 0; i < MAX_PARTITIONS; i++) partitions[i].owner_pid = -1;
    partition_count = 0;
}

int memory_find_slot(const char *name)
{
    for (int i = 0; i < prog_slot_count; i++) {
        if (prog_slots[i].in_use && strcmp(prog_slots[i].name, name) == 0)
            return i;
    }
    return -1;
}

/* Find the first free region of 'needed' consecutive slots in memory[].
   Returns start index or -1. */
static int find_free_region(int needed)
{
    int i = 0;
    while (i <= MEMORY_SIZE - needed) {
        /* Check if memory[i..i+needed-1] is clear */
        int ok = 1;
        for (int j = i; j < i + needed; j++) {
            if (memory[j].op != INS_NOP) { ok = 0; break; }
        }
        if (ok) return i;
        i++;
    }
    return -1;
}

void memory_compact(PCB *pcb_table, int pcb_count)
{
    /* Build ordered list of live slots by start address */
    int order[MAX_PROG_SLOTS];
    int n = 0;
    for (int i = 0; i < prog_slot_count; i++) {
        if (prog_slots[i].in_use) order[n++] = i;
    }
    /* Bubble sort by start */
    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            if (prog_slots[order[j]].start < prog_slots[order[i]].start) {
                int tmp = order[i]; order[i] = order[j]; order[j] = tmp;
            }
        }
    }

    int cursor = 0;
    for (int k = 0; k < n; k++) {
        ProgSlot *s = &prog_slots[order[k]];
        if (s->start != cursor) {
            memmove(&memory[cursor], &memory[s->start],
                    s->length * sizeof(Instruction));
            /* Clear old region */
            for (int j = cursor + s->length; j < s->start + s->length; j++)
                memory[j].op = INS_NOP;
            /* Patch PCBs */
            for (int p = 0; p < pcb_count; p++) {
                if (pcb_table[p].active &&
                    pcb_table[p].memory_start == s->start) {
                    pcb_table[p].memory_start = cursor;
                }
            }
            s->start = cursor;
        }
        cursor += s->length;
    }
    /* Clear remainder */
    for (int i = cursor; i < MEMORY_SIZE; i++)
        memory[i].op = INS_NOP;
}

int memory_load_program(const char *name, PCB *pcb_table, int pcb_count)
{
    int slot = memory_find_slot(name);
    if (slot >= 0) {
        prog_slots[slot].ref_count++;
        return prog_slots[slot].start;
    }

    /* Count instructions needed */
    int needed = loader_count_instructions(name);
    if (needed <= 0) {
        fprintf(stderr, "loader: cannot count instructions in '%s'\n", name);
        return -1;
    }

    int start = find_free_region(needed);
    if (start < 0) {
        /* Try compaction once */
        memory_compact(pcb_table, pcb_count);
        start = find_free_region(needed);
        if (start < 0) {
            fprintf(stderr, "memory: out of space for '%s' (%d slots needed)\n",
                    name, needed);
            return -1;
        }
    }

    /* Find a free slot entry */
    int si = -1;
    for (int i = 0; i < MAX_PROG_SLOTS; i++) {
        if (!prog_slots[i].in_use) { si = i; break; }
    }
    if (si < 0) {
        fprintf(stderr, "memory: prog_slots table full\n");
        return -1;
    }
    if (si >= prog_slot_count) prog_slot_count = si + 1;

    int mem_size_hint = 0;
    int loaded = loader_load(name, memory, MEMORY_SIZE, start, &mem_size_hint);
    if (loaded < 0) return -1;

    strncpy(prog_slots[si].name, name, MAX_NAME - 1);
    prog_slots[si].start     = start;
    prog_slots[si].length    = loaded;
    prog_slots[si].mem_size  = mem_size_hint;
    prog_slots[si].ref_count = 1;
    prog_slots[si].in_use    = 1;

    return start;
}

void memory_release(const char *name, PCB *pcb_table, int pcb_count)
{
    int slot = memory_find_slot(name);
    if (slot < 0) return;

    prog_slots[slot].ref_count--;
    if (prog_slots[slot].ref_count <= 0) {
        int s = prog_slots[slot].start;
        int l = prog_slots[slot].length;
        for (int i = s; i < s + l && i < MEMORY_SIZE; i++)
            memory[i].op = INS_NOP;
        prog_slots[slot].in_use    = 0;
        prog_slots[slot].ref_count = 0;
        (void)pcb_table; (void)pcb_count;
    }
}

void memory_dump(void)
{
    printf("--- MEMORY SLOTS ---\n");
    for (int i = 0; i < prog_slot_count; i++) {
        if (prog_slots[i].in_use) {
            printf("  [%d] '%s' start=%d len=%d mem_size=%d refs=%d\n",
                   i, prog_slots[i].name, prog_slots[i].start,
                   prog_slots[i].length, prog_slots[i].mem_size,
                   prog_slots[i].ref_count);
        }
    }
    if (partition_count > 0) {
        printf("--- PARTITIONS ---\n");
        for (int i = 0; i < partition_count; i++) {
            printf("  [%d] size=%d owner_pid=%d\n",
                   i, partitions[i].size, partitions[i].owner_pid);
        }
    }
    printf("--------------------\n");
}

/* ------------------------------------------------------------------ */
/*  Partition management                                               */
/* ------------------------------------------------------------------ */

void partitions_init(int total_mem, int n_parts)
{
    if (n_parts <= 0 || n_parts > MAX_PARTITIONS) return;
    partition_count = n_parts;
    int sz = total_mem / n_parts;
    for (int i = 0; i < n_parts; i++) {
        partitions[i].size      = sz;
        partitions[i].owner_pid = -1;
    }
}

int partition_alloc(int pid, int size)
{
    /* first-fit */
    for (int i = 0; i < partition_count; i++) {
        if (partitions[i].owner_pid == -1 && partitions[i].size >= size) {
            partitions[i].owner_pid = pid;
            return i;
        }
    }
    return -1;
}

void partition_release(int pid)
{
    for (int i = 0; i < partition_count; i++) {
        if (partitions[i].owner_pid == pid)
            partitions[i].owner_pid = -1;
    }
}

int partitions_enabled(void) { return partition_count > 0; }
