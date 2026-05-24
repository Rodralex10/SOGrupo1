#ifndef LOADER_H
#define LOADER_H

#include "types.h"

/*
 * Load a .prg file into the instruction buffer starting at 'start'.
 * Returns the number of instructions loaded, or -1 on error.
 * 'mem_size_out' receives the logical memory size declared on line 1
 * (0 if not present / not a number).
 */
int loader_load(const char *filename, Instruction *buf, int buf_size,
                int start, int *mem_size_out);

/* Count the number of instructions in a .prg file (for SJF remaining) */
int loader_count_instructions(const char *filename);

#endif /* LOADER_H */
