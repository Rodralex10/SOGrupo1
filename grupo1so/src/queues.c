/*
 * queues.c — Fila FIFO e fila de prioridade (min-heap)
 */
#include <stdio.h>
#include <string.h>
#include "queues.h"

/* ========== Fila FIFO ========== */

void fifo_init(FifoQueue *q)
{
    q->head  = 0;
    q->tail  = 0;
    q->count = 0;
    memset(q->data, -1, sizeof(q->data));
}

int fifo_enqueue(FifoQueue *q, int idx)
{
    if (q->count >= MAX_PROCESSES) return -1;
    q->data[q->tail] = idx;
    q->tail = (q->tail + 1) % MAX_PROCESSES;
    q->count++;
    return 0;
}

int fifo_dequeue(FifoQueue *q)
{
    if (q->count == 0) return -1;
    int idx = q->data[q->head];
    q->head = (q->head + 1) % MAX_PROCESSES;
    q->count--;
    return idx;
}

int fifo_peek(const FifoQueue *q)
{
    if (q->count == 0) return -1;
    return q->data[q->head];
}

/* Remove um índice específico (mantém ordem dos restantes) */
int fifo_remove(FifoQueue *q, int idx)
{
    for (int i = 0; i < q->count; i++) {
        int pos = (q->head + i) % MAX_PROCESSES;
        if (q->data[pos] == idx) {
            for (int j = i; j < q->count - 1; j++) {
                int cur  = (q->head + j)     % MAX_PROCESSES;
                int next = (q->head + j + 1) % MAX_PROCESSES;
                q->data[cur] = q->data[next];
            }
            q->tail = (q->tail - 1 + MAX_PROCESSES) % MAX_PROCESSES;
            q->count--;
            return 0;
        }
    }
    return -1;
}

int fifo_size(const FifoQueue *q) { return q->count; }

void fifo_print(const FifoQueue *q, const char *label)
{
    printf("%s [%d]: ", label, q->count);
    for (int i = 0; i < q->count; i++) {
        printf("%d ", q->data[(q->head + i) % MAX_PROCESSES]);
    }
    printf("\n");
}

/* ========== Fila de prioridade (min-heap) ========== */

void prio_init(PrioQueue *q, PQCmp cmp, const PCB *tbl)
{
    q->count = 0;
    q->cmp   = cmp;
    q->tbl   = tbl;
}

static void heap_swap(PrioQueue *q, int i, int j)
{
    int tmp = q->data[i]; q->data[i] = q->data[j]; q->data[j] = tmp;
}

/* Sobe o elemento até estar na posição correcta do heap */
static void sift_up(PrioQueue *q, int i)
{
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (q->cmp(q->data[i], q->data[parent], q->tbl) < 0) {
            heap_swap(q, i, parent);
            i = parent;
        } else break;
    }
}

/* Desce o elemento até estar na posição correcta */
static void sift_down(PrioQueue *q, int i)
{
    int n = q->count;
    while (1) {
        int smallest = i;
        int l = 2*i+1, r = 2*i+2;
        if (l < n && q->cmp(q->data[l], q->data[smallest], q->tbl) < 0)
            smallest = l;
        if (r < n && q->cmp(q->data[r], q->data[smallest], q->tbl) < 0)
            smallest = r;
        if (smallest == i) break;
        heap_swap(q, i, smallest);
        i = smallest;
    }
}

int prio_push(PrioQueue *q, int idx)
{
    if (q->count >= MAX_PROCESSES) return -1;
    q->data[q->count++] = idx;
    sift_up(q, q->count - 1);
    return 0;
}

int prio_pop(PrioQueue *q)
{
    if (q->count == 0) return -1;
    int top = q->data[0];
    q->data[0] = q->data[--q->count];
    sift_down(q, 0);
    return top;
}

int prio_peek(const PrioQueue *q)
{
    return (q->count > 0) ? q->data[0] : -1;
}

int prio_remove(PrioQueue *q, int idx)
{
    for (int i = 0; i < q->count; i++) {
        if (q->data[i] == idx) {
            q->data[i] = q->data[--q->count];
            sift_up(q, i);
            sift_down(q, i);
            return 0;
        }
    }
    return -1;
}

int prio_size(const PrioQueue *q) { return q->count; }

/* Usado no SJF quando remaining muda (instrução L) */
void prio_rebuild(PrioQueue *q)
{
    for (int i = q->count / 2 - 1; i >= 0; i--)
        sift_down(q, i);
}
