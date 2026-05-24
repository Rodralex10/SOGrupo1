/*
 * loader.c — Leitura e interpretação de ficheiros .prg
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "loader.h"
#include "types.h"

/* Abre o ficheiro na pasta actual ou em data/programs/ */
static FILE *open_program(const char *filename, char *resolved, int rsz)
{
    FILE *f = fopen(filename, "r");
    if (f) { strncpy(resolved, filename, rsz - 1); return f; }

    char path[256];
    snprintf(path, sizeof(path), "data/programs/%s", filename);
    f = fopen(path, "r");
    if (f) { strncpy(resolved, path, rsz - 1); return f; }

    fprintf(stderr, "loader: cannot open '%s'\n", filename);
    return NULL;
}

/*
 * Converte uma linha do .prg numa Instruction.
 * Devolve 1 se leu instrução, 0 para ignorar (vazia/comentário).
 */
static int parse_line(const char *line, Instruction *ins)
{
    while (isspace((unsigned char)*line)) line++;
    if (*line == '\0' || *line == '#') return 0;

    ins->op   = INS_NOP;
    ins->n    = 0;
    ins->nome[0] = '\0';

    char op = (char)toupper((unsigned char)line[0]);
    switch (op) {
    case 'M': case 'A': case 'S':
        ins->op = (InsOp)op;
        ins->n  = atoi(line + 1);
        return 1;
    case 'B': case 'T':
        ins->op = (InsOp)op;
        return 1;
    case 'C':
        ins->op = (InsOp)op;
        ins->n  = atoi(line + 1);
        return 1;
    case 'L': {
        ins->op = (InsOp)op;
        const char *p = line + 1;
        while (isspace((unsigned char)*p)) p++;
        strncpy(ins->nome, p, MAX_NAME - 1);
        int len = (int)strlen(ins->nome);
        while (len > 0 && isspace((unsigned char)ins->nome[len-1]))
            ins->nome[--len] = '\0';
        return 1;
    }
    default:
        return 0;
    }
}

int loader_load(const char *filename, Instruction *buf, int buf_size,
                int start, int *mem_size_out)
{
    char resolved[256] = {0};
    FILE *f = open_program(filename, resolved, sizeof(resolved));
    if (!f) return -1;

    *mem_size_out = 0;
    int count = 0;
    char line[256];
    int first_line = 1;

    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = '\0';

        /* A 1.ª linha pode ser só um inteiro = tamanho lógico em memória */
        if (first_line) {
            first_line = 0;
            char *end;
            long v = strtol(line, &end, 10);
            while (isspace((unsigned char)*end)) end++;
            if (*end == '\0' && end != line) {
                *mem_size_out = (int)v;
                continue;
            }
        }

        if (start + count >= buf_size) {
            fprintf(stderr, "loader: memory buffer full while loading '%s'\n",
                    filename);
            fclose(f);
            return count;
        }

        Instruction ins;
        int r = parse_line(line, &ins);
        if (r == 1) {
            buf[start + count] = ins;
            count++;
        }
    }

    fclose(f);
    return count;
}

/* Igual a loader_load mas só conta (não escreve no buffer) */
int loader_count_instructions(const char *filename)
{
    char resolved[256] = {0};
    FILE *f = open_program(filename, resolved, sizeof(resolved));
    if (!f) return -1;

    int count = 0;
    char line[256];
    int first_line = 1;

    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (first_line) {
            first_line = 0;
            char *end;
            strtol(line, &end, 10);
            while (isspace((unsigned char)*end)) end++;
            if (*end == '\0' && end != line) continue;
        }
        Instruction ins;
        if (parse_line(line, &ins) == 1) count++;
    }

    fclose(f);
    return count;
}
