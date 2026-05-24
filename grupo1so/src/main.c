/*
 * main.c — Ponto de entrada do simulador
 */
#include "manager.h"

int main(int argc, char *argv[])
{
    manager_init(argc, argv);   /* configura memória, filas e plano */
    manager_run();              /* lê comandos até T ou fim do ficheiro */
    return 0;
}