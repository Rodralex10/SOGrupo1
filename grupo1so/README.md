# Simulador de Gestão de Processos

Trabalho prático da cadeira de **Sistemas Operativos**, que simula o escalonamento de CPU, criação e termina processos e também gestão de memória!


## Compilação

Requer GCC + make num sistema POSIX (Linux, macOS, WSL no Windows):

```bash
make          # compila e gera ./sim
make clean    # remove ficheiros objecto e executável
make test     # compila e corre os testes FCFS e Priority
```

## Execução

```bash
./sim [opções]
```

| Opção | Descrição | Exemplo |
|---|---|---|
| `--scheduler <algo>` | Algoritmo de escalonamento | `fcfs` `priority` `sjf` `rm` `edf` |
| `--quantum <n>` | Time quantum (default 4) | `--quantum 3` |
| `--plan <ficheiro>` | Ficheiro de processos iniciais | `--plan data/plan.txt` |
| `--control <ficheiro>` | Comandos de controlo (`-` = stdin) | `--control data/control.txt` |
| `--no-preempt` | Modo não-preemptivo | |
| `--memory <n>` | Tamanho total de memória lógica | `--memory 1000` |
| `--partitions <n>` | Número de partições de memória | `--partitions 4` |

Sem opções, o simulador lê `data/config.txt` (ver `config.txt.example`).

## Ficheiros de entrada

### `data/plan.txt`
Um processo por linha: `programa.prg  chegada  [prioridade  [periodo  deadline]]`

```
p1.prg   0  4
```

### `data/control.txt` / stdin
| Comando | Acção |
|---|---|
| `E` | Executar um quantum |
| `I` | Interromper e bloquear processo em execução |
| `D` | Escalonador de longo prazo (desbloquear processo aleatório) |
| `R` | Relatório do estado actual |
| `T` | Terminar simulador e imprimir estatísticas globais |

### Programas `.prg`
Cada ficheiro é uma sequência de instruções (uma por linha):

| Instrução | Semântica |
|---|---|
| `M n` | `var = n` |
| `A n` | `var += n` |
| `S n` | `var -= n` |
| `B` | Bloquear este processo |
| `T` | Terminar este processo |
| `C n` | Fork: filho começa na instrução seguinte; pai salta `n` instruções |
| `L ficheiro` | Substituir programa actual (exec) |

A primeira linha pode ser um inteiro indicando o tamanho lógico de memória necessário (parte opcional).

## Algoritmos de escalonamento implementados

| Nome | Selector | Preempção |
|---|---|---|
| `fcfs` | FIFO | Fim de quantum / B / T |
| `priority` | Menor número = maior prioridade | Imediata ao chegar processo mais prioritário |
| `sjf` | Menor nº de instruções restantes | Imediata ao chegar processo mais curto |
| `rm` | Rate Monotonic — menor período | Periódica |
| `edf` | Earliest Deadline First | Imediata |

## Gestão de memória

O array `memory[1000]` aloja as instruções de todos os programas em simultâneo.  
Quando não há espaço contíguo livre, é feita **compactação** automática.  
Com partições activadas (`--partitions n`), cada processo precisa de uma partição de tamanho `memory_total / n`; se não houver, o processo fica em `WAIT_MEM` até que `D` seja invocado.

## Estrutura de ficheiros

```
trabalho/
├── Makefile
├── README.md
├── config.txt.example
├── include/          # Cabeçalhos (.h)
├── src/              # Código fonte (.c)
├── data/
│   ├── plan.txt
│   ├── control.txt
│   ├── config.txt
│   └── programs/     # *.prg
```

## Exemplo rápido

```bash
# Exemplo do enunciado: p1.prg lança filho1.prg e filho2.prg
./sim --scheduler fcfs --quantum 4

# Testar prioridades com preempção
./sim --scheduler priority --plan data/plan_prio.txt --control data/control.txt

# Testar bloqueio/desbloqueio
./sim --plan data/plan_block.txt --control data/control_block.txt
```
