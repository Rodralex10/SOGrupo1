#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "loader.h"
#include "types.h"

/* Search for 'filename' first in current dir, then in data/programs/ */
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
 * Parse a single non-empty, non-comment line into an Instruction.
 * Returns 1 on success, 0 to skip, -1 on fatal parse error.
 */
static int parse_line(const char *line, Instruction *ins)
{
    /* Skip whitespace */
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
        /* Trim trailing whitespace */
        int len = (int)strlen(ins->nome);
        while (len > 0 && isspace((unsigned char)ins->nome[len-1]))
            ins->nome[--len] = '\0';
        return 1;
    }
    default:
        return 0; /* unknown / comment */
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
        /* Strip newline */
        line[strcspn(line, "\r\n")] = '\0';

        /* First non-empty line may be an optional memory size integer */
        if (first_line) {
            first_line = 0;
            char *end;
            long v = strtol(line, &end, 10);
            /* If the whole line is a number, treat as memory size */
            while (isspace((unsigned char)*end)) end++;
            if (*end == '\0' && end != line) {
                *mem_size_out = (int)v;
                continue; /* not an instruction */
            }
        }

        if (start + count >= buf_size) {
            fprintf(stderr, "loader: memory buffer full while loading '%s'\n",
                    filename);
            fclose(f);
            return count; /* return what we have */
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
