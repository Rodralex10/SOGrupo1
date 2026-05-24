/*
 * loader.h — Carregamento de programas .prg
 */
#ifndef LOADER_H
#define LOADER_H

#include "types.h"

/* Carrega .prg em buf[start..]; devolve nº de instruções ou -1 em erro */
int loader_load(const char *filename, Instruction *buf, int buf_size,
                int start, int *mem_size_out);

/* Conta instruções (para reservar espaço antes de carregar) */
int loader_count_instructions(const char *filename);

#endif /* LOADER_H */
