# 📁 PROJECT: MiniOS — Shell Interactivo con Gestión de Procesos, Hilos y Memoria

> **Curso:** Sistemas Operativos | **Plataforma:** Linux | **Prompt:** `miniOS> `

---

## 🗺️ Ruta Crítica Reorganizada (Critical Path)

```
[M1] Infraestructura Base (Shell REPL + Parser + Dispatcher)
        │
        ├──────────────────────────┬──────────────────────────┐
        ▼                          ▼                          ▼
[M2] Módulo 1 — Procesos   [M3] Módulo 3 — Memoria    [M7] Módulo 2 — Hilos
  fork / exec / wait / ps     Config + First-Fit          pthread Productor/Consumidor
                                   │
                           [M4] Módulo 3 — Cola RAM
                                mem_queue (Espera)
                                   │
                           [M5] Módulo 3 — Paginación
                                Marcos + Traducción
                                   │
                           [M6] Módulo 3 — Visualizador
                                mem-view + switch-ram seguro
        │                          │                          │
        └──────────────────────────┴──────────────────────────┘
                                   │
                           [M8] Integración Global + Tests + Valgrind
                                   │
                           [M9] Entregables Finales (Informe, README, Video)
```

> **Ruta Crítica:** M1 → M3 → M4 → M5 → M6 → M8 → M9
> - **Paralelismo:** Tras completar **M1**, se pueden desarrollar en paralelo **M2** (Procesos), **M3** (Memoria FF) y **M7** (Hilos).
> - **Secuencia de Memoria:** El Módulo 3 se descompone en 4 etapas lógicas e incrementales (**M3** Configuración/FF → **M4** Cola de Espera RAM → **M5** Paginación → **M6** Visualizador terminal y Switch seguro).

---

## 📊 Resumen General de Milestones y Módulos

| Milestone | Módulo Correspondiente | Nombre del Milestone | Issues Clave | Dependencias | Etapa Git Sugerida |
|:---:|:---|:---|:---:|:---:|:---:|
| **M1** | Shell / CLI Base | **Infraestructura Base y Shell CLI** | 1.1 – 1.4 | — | `Commit 1: feat: bootstrap shell base` |
| **M2** | Módulo 1: Procesos | **Gestión de Procesos y Concurrencia** | 2.1 – 2.4 | M1 | `Commit 2: feat: module-1 processes` |
| **M3** | Módulo 3: Memoria (I) | **Configuración de RAM, Presets y First-Fit** | 3.1 – 3.4 | M1 | `Commit 3: feat: module-3 memory first-fit` |
| **M4** | Módulo 3: Memoria (II) | **Cola de Espera por RAM Insuficiente (mem_queue)** | 4.1 – 4.4 | M3 | `Commit 3.1: feat: memory waiting queue` |
| **M5** | Módulo 3: Memoria (III) | **Paginación y Traducción de Direcciones** | 5.1 – 5.3 | M3, M4 | `Commit 3.2: feat: memory paging translation` |
| **M6** | Módulo 3: Memoria (IV) | **Visualizador Terminal (mem-view) y Switch Seguro** | 6.1 – 6.3 | M3, M4, M5 | `Commit 3.3: feat: memory visualizer and switch-ram` |
| **M7** | Módulo 2: Hilos | **Hilos: Productor-Consumidor con pthread** | 7.1 – 7.4 | M1 | `Commit 4: feat: module-2 threads producer-consumer` |
| **M8** | Sistema e Integración | **Integración Global, Señales y Hardening** | 8.1 – 8.4 | M2, M6, M7 | `Commit 4.1: test: system integration and leaks check` |
| **M9** | Documentación / Entrega | **Entregables Finales: README, Informe y Video** | 9.1 – 9.3 | M8 | `Commit 5: docs: final deliverables and release` |

---

## 🚩 MILESTONE 1: Infraestructura Base — Shell CLI y Repositorio

> **Módulo:** Shell / CLI Base
> **Objetivo:** Establecer el repositorio Git, estructurar directorios modulares, compilar el ejecutable principal con ciclo REPL (`miniOS> `), tokenizador de comandos y dispatcher central.
> 🏷️ **Etapa Git:** Commit 1 — `"feat: bootstrap shell base"` | ⏱️ Duración estimada: ~3 días

---

### ├─ 📝 Issue 1.1: Inicializar repositorio Git y estructura de carpetas
**Descripción:**
Configurar el repositorio Git con estructura desacoplada para los 3 módulos del sistema, `.gitignore` para artefactos en C, `Makefile` automatizado y documentación inicial.

**Estructura de carpetas:**
```
mini-OS/
├── src/
│   ├── main.c
│   ├── shell/          # CLI, REPL, parser, dispatcher
│   │   ├── shell.c / shell.h
│   │   └── parser.c / parser.h
│   ├── process/        # Módulo 1: Procesos
│   │   ├── process.c / process.h
│   │   └── signals.c / signals.h
│   ├── memory/         # Módulo 3: Memoria
│   │   ├── memory.c / memory.h
│   │   ├── first_fit.c / first_fit.h
│   │   ├── paging.c / paging.h
│   │   └── mem_queue.c / mem_queue.h  # Feature: Cola de espera RAM
│   └── threads/        # Módulo 2: Hilos
│       ├── buffer.c / buffer.h
│       └── producer_consumer.c / producer_consumer.h
├── docs/
│   ├── PROJECT_MANAGEMENT.md
│   └── use-cases/
├── tests/
├── Makefile
├── .gitignore
└── README.md
```

**Criterios de aceptación:**
- [ ] Repositorio Git inicializado con branch principal `main`.
- [ ] `.gitignore` filtra `*.o`, `mini-os`, temporales y reportes de Valgrind.
- [ ] `Makefile` con reglas `all`, `clean`, `run` y flags estrictos (`-Wall -Wextra -std=c11 -lpthread`).

**🔗 Depende de:** — *(Punto de partida)*

---

### ├─ 📝 Issue 1.2: Implementar el ciclo principal del shell (REPL)
**Descripción:**
Implementar en `src/shell/shell.c` el ciclo interactivo Read-Eval-Print-Loop con prompt `miniOS> `. Debe procesar saltos de línea vacíos, capturar EOF (`Ctrl+D`) y terminar limpiamente con código 0.

**Implementación de referencia:**
```c
void shell_loop(void) {
    char line[MAX_LINE];
    while (1) {
        printf("miniOS> ");
        fflush(stdout);
        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n[INFO] Saliendo del shell...\n");
            break; // EOF (Ctrl+D)
        }
        if (strlen(line) <= 1) continue; // Enter vacío
        shell_dispatch(line);
    }
}
```

**Criterios de aceptación:**
- [ ] Muestra el prompt `miniOS> ` en cada comando.
- [ ] Manejo correcto de EOF (`Ctrl+D`) sin bucles infinitos.
- [ ] Comando `exit` finaliza la aplicación con retorno 0.

**🔗 Depende de:** Issue 1.1

---

### ├─ 📝 Issue 1.3: Parser y tokenizador de línea de comandos
**Descripción:**
Desarrollar `src/shell/parser.c` para fragmentar cadenas de entrada en un vector `argv[]` delimitado por espacios, tabuladores y saltos de línea, gestionando memoria dinámica de forma segura.

**Estructura y prototipo:**
```c
#define MAX_ARGS 64
#define MAX_LINE 1024

typedef struct {
    char *argv[MAX_ARGS];
    int argc;
    int is_background; // Flag detectado por '&'
} Command;

int parse_command(const char *line, Command *cmd);
void free_command(Command *cmd);
```

**Criterios de aceptación:**
- [ ] Ignora espacios en blanco múltiples y tabulaciones consecutivas.
- [ ] Detecta el token final `&` para marcar ejecución en segundo plano.
- [ ] Libera toda la memoria asignada en `free_command` (0 leaks).

**🔗 Depende de:** Issue 1.2

---

### ├─ 📝 Issue 1.4: Dispatcher central y comandos built-in básicos
**Descripción:**
Construir la tabla de despacho en `src/shell/shell.c` para enrutar comandos a sus módulos correspondientes y proveer comandos internos básicos (`help`, `clear`, `exit`).

**Comandos mínimos:**
- `help`: Lista comandos agrupados por módulo.
- `clear`: Limpia la consola con secuencias ANSI (`\033[H\033[J`).
- `exit`: Termina la sesión del shell.

**Criterios de aceptación:**
- [ ] Enrutamiento modular sin acoplamiento excesivo.
- [ ] Si el comando no es built-in ni ejecutable externo, emite: `miniOS: comando no encontrado: '<nombre>'`.

**🔗 Depende de:** Issue 1.3

---

## 🚩 MILESTONE 2: Módulo 1 — Gestión de Procesos y Concurrencia

> **Módulo:** Módulo 1: Procesos
> **Objetivo:** Ejecutar comandos de sistema en Linux mediante `fork()`, `execvp()` y `waitpid()`, soportar ejecución en foreground y background (`&`), y gestionar la tabla de procesos con `ps-mini`.
> 🏷️ **Etapa Git:** Commit 2 — `"feat: module-1 processes"` | ⏱️ Duración estimada: ~4 días

---

### ├─ 📝 Issue 2.1: Ejecución foreground con fork, execvp y waitpid
**Descripción:**
Implementar en `src/process/process.c` la ejecución síncrona de comandos externos. El padre suspende su ejecución con `waitpid` hasta que el proceso hijo finalice, reportando el código de retorno.

**Lógica clave:**
```c
int execute_foreground(Command *cmd) {
    pid_t pid = fork();
    if (pid < 0) { perror("fork falló"); return -1; }
    if (pid == 0) {
        // Proceso hijo
        if (execvp(cmd->argv[0], cmd->argv) < 0) {
            fprintf(stderr, "miniOS: error al ejecutar '%s': %s\n", 
                    cmd->argv[0], strerror(errno));
            exit(127);
        }
    } else {
        // Proceso padre
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
    }
    return 0;
}
```

**Criterios de aceptación:**
- [ ] Comandos estándar de Linux (`ls`, `uname`, `grep`, etc.) funcionan correctamente.
- [ ] El shell recupera el control inmediatamente tras finalizar el proceso hijo.

**🔗 Depende de:** Issue 1.4

---

### ├─ 📝 Issue 2.2: Ejecución en background con operador & y reserva de memoria
**Descripción:**
Permitir que un comando terminado en `&` (e.g. `sleep 10 256 &` o `sleep 10 &`) se ejecute de manera asíncrona. El shell valida primero el espacio disponible en la RAM simulada. Si hay espacio, reserva el bloque (o marcos) y lanza el proceso sin esperar con `waitpid()` bloqueante; si no hay espacio, genera una alerta de RAM insuficiente y encola el proceso en `mem_queue`.

**Comportamiento esperado:**
```
miniOS> sleep 10 256 &
[RAM ALLOC] PID 24891 asignado en RAM: 256 KB
[BG] Proceso lanzado en background con PID: 24891 | RAM: 256 KB
miniOS> 
```

**Criterios de aceptación:**
- [ ] No bloquea el terminal para que el usuario continúe interactuando.
- [ ] Reserva memoria en el subsistema de RAM (`ff_alloc` o `pag_alloc`).
- [ ] Si la memoria es insuficiente, emite alerta y encola en `mem_queue` sin tumbar el shell.
- [ ] Al finalizar el proceso (`SIGCHLD`), libera automáticamente la memoria asignada y aplica coalescencia.

**🔗 Depende de:** Issue 2.1

---

### ├─ 📝 Issue 2.3: Tabla de control de procesos en background
**Descripción:**
Mantener una tabla dinámica/array de procesos lanzados (`bg_table`) para registrar su PID, PPID, comando ejecutado, memoria ocupada en KB y estado (`RUNNING`, `FINISHED`, `TERMINATED`).

**Estructura:**
```c
typedef enum { PROC_RUNNING, PROC_FINISHED, PROC_KILLED } ProcessStatus;

typedef struct {
    pid_t pid;
    pid_t ppid;
    char command[128];
    int mem_kb;                 // Memoria ocupada en RAM (KB)
    ProcessStatus status;
    int exit_code;
} BgProcess;
```

**Criterios de aceptación:**
- [ ] Almacena hasta un límite configurado de procesos concurrentes (ej. 32).
- [ ] Registra y actualiza la cantidad de KB ocupados por cada proceso.
- [ ] Actualiza el estado sin bloquear el shell.

**🔗 Depende de:** Issue 2.2

---

### ├─ 📝 Issue 2.4: Comando ps-mini — Visualizador de procesos de la sesión
**Descripción:**
Implementar el comando interno `ps-mini` que visualiza la lista de procesos creados desde el inicio del shell con sus metadatos en formato tabular estructurado, incluyendo la memoria RAM reservada.

**Salida en terminal:**
```
miniOS> ps-mini
╔════╦════════╦════════╦══════════════════════╦══════════════╦══════════════╗
║ #  ║ PID    ║ PPID   ║ Comando              ║ RAM (KB)     ║ Estado       ║
╠════╬════════╬════════╬══════════════════════╬══════════════╬══════════════╣
║ 1  ║ 24891  ║ 24800  ║ sleep 10             ║ 256 KB       ║ RUNNING      ║
║ 2  ║ 24895  ║ 24800  ║ ls -la               ║ 128 KB       ║ DONE (0)     ║
╚════╩════════╩════════╩══════════════════════╩══════════════╩══════════════╝
```

**Criterios de aceptación:**
- [ ] Muestra columnas: Número, PID, PPID, Comando, RAM (KB) y Estado.
- [ ] Diferencia entre procesos en ejecución y finalizados (liberados).

**🔗 Depende de:** Issue 2.3

---

## 🚩 MILESTONE 3: Módulo 3 — Memoria: Configuración Global, Presets y Algoritmo First-Fit

> **Módulo:** Módulo 3: Memoria (Simulación y Particionado First-Fit)
> **Objetivo:** Establecer la infraestructura de memoria simulada, configuración de RAM total, marcos y tamaño de página, presets por defecto (`ff default marcos`), y la asignación/liberación contigua First-Fit con coalescencia.
> 🏷️ **Etapa Git:** Commit 3 — `"feat: module-3 memory first-fit"` | ⏱️ Duración estimada: ~4 días

---

### ├─ 📝 Issue 3.1: Estructuras de datos de memoria y lista de bloques
**Descripción:**
Diseñar en `src/memory/memory.h` las estructuras que modelan la memoria física simulada mediante una lista enlazada ordenada por direcciones base de bloques para Particionamiento Dinámico Contiguo.

**Estructuras de referencia:**
```c
typedef enum { BLOCK_FREE = 0, BLOCK_USED = 1 } BlockStatus;

typedef struct MemBlock {
    int id;
    int start_kb;
    int size_kb;
    BlockStatus status;
    int process_id;             // -1 si libre
    struct MemBlock *next;
} MemBlock;

typedef struct {
    int total_ram_kb;
    int page_size_kb;
    int total_frames;
    char algorithm[8];          // "ff" (First-Fit contiguo) o "pag" (Paginación)
    MemBlock *block_list;
} MemoryState;

extern MemoryState mem_state;
void mem_init(void);
void mem_reset_blocks(void);
```

**Criterios de aceptación:**
- [ ] Inicializa la RAM en First-Fit como un único bloque gigante `BLOCK_FREE` de tamaño `total_ram_kb`.
- [ ] La lista enlazada mantiene siempre orden ascendente por `start_kb`.
- [ ] En modo First-Fit la memoria opera como espacio contiguo lineal sin requerir marcos ni páginas.

**🔗 Depende de:** Issue 1.4

---

### ├─ 📝 Issue 3.2: Comandos de configuración de RAM y presets
**Descripción:**
Implementar comandos para configurar el tamaño general de RAM y los parámetros latentes de marcos/páginas, así como el comando preset `mem-config default` para establecer valores predeterminados seguros para la evaluación.

**Comandos de terminal:**
```
miniOS> mem-config ram <KB>          # Define tamaño general de RAM e inicializa bloque gigante libre
miniOS> mem-config page <KB>         # Parámetro latente para Paginación (potencia de 2, ej. 64 KB)
miniOS> mem-config frames <N>        # Cantidad de marcos calculada / configurada para Paginación
miniOS> mem-config default           # Preset: 1024 KB RAM, 64 KB página (16 marcos latentes), Algoritmo FF activo
miniOS> mem-status                   # Muestra resumen adaptado al algoritmo activo (bloques en FF, marcos en PAG)
```

**Validaciones requeridas:**
- `ram_kb` debe ser múltiplo del `page_size_kb`.
- `page_size_kb` debe ser potencia de 2 (`32`, `64`, `128`, etc.).
- Cálculo automático de consistencia: `total_frames = total_ram_kb / page_size_kb`.
- No se permite reconfigurar si hay procesos activos en memoria (exige liberación previa).
- Bloqueo de comandos incompatibles: si el algoritmo activo es `ff`, los comandos `mem-page-table` y `mem-translate` son rechazados con mensaje explicativo.

**Criterios de aceptación:**
- [ ] Parámetros configurables individualmente o con preset por defecto.
- [ ] `mem-status` muestra el estado específico del algoritmo activo (bloque gigante continuo en FF, marcos en PAG).
- [ ] En modo First-Fit se bloquea la invocación de operaciones propias de Paginación.

**🔗 Depende de:** Issue 3.1

---

### ├─ 📝 Issue 3.3: Algoritmo First-Fit — Asignación contigua (ff_alloc) y split
**Descripción:**
Implementar en `src/memory/first_fit.c` la búsqueda del primer bloque libre con capacidad suficiente. Si el bloque es mayor a lo solicitado, se realiza una partición (*split*).

**Lógica de asignación:**
```c
int ff_alloc(int pid, int size_kb) {
    MemBlock *curr = mem_state.block_list;
    while (curr != NULL) {
        if (curr->status == BLOCK_FREE && curr->size_kb >= size_kb) {
            if (curr->size_kb > size_kb) {
                // Split del bloque
                MemBlock *resto = malloc(sizeof(MemBlock));
                resto->id = ++block_counter;
                resto->start_kb = curr->start_kb + size_kb;
                resto->size_kb = curr->size_kb - size_kb;
                resto->status = BLOCK_FREE;
                resto->process_id = -1;
                resto->next = curr->next;

                curr->size_kb = size_kb;
                curr->next = resto;
            }
            curr->status = BLOCK_USED;
            curr->process_id = pid;
            printf("[FF] PID %d asignado en dirección %d KB (%d KB)\n", pid, curr->start_kb, size_kb);
            return 0; // Asignación exitosa
        }
        curr = curr->next;
    }
    // Memoria insuficiente o fragmentación externa
    return -1;
}
```

**Criterios de aceptación:**
- [ ] Asigna siempre el primer bloque libre que satisface el tamaño.
- [ ] Realiza split correcto sin perder bytes entre bloques.
- [ ] Retorna -1 si no existe bloque contiguo suficiente (señal para encolado en `mem_queue`).

**🔗 Depende de:** Issue 3.2

---

### ├─ 📝 Issue 3.4: Algoritmo First-Fit — Liberación y coalescencia de bloques
**Descripción:**
Implementar `ff_free(pid)` que marca el bloque como libre y fusiona automáticamente bloques libres contiguos adyacentes (*coalescencia* o *merging*) para mitigar la fragmentación externa.

**Coalescencia de bloques adyacentes:**
```c
void merge_adjacent_blocks(void) {
    MemBlock *curr = mem_state.block_list;
    while (curr && curr->next) {
        if (curr->status == BLOCK_FREE && curr->next->status == BLOCK_FREE) {
            MemBlock *temp = curr->next;
            curr->size_kb += temp->size_kb;
            curr->next = temp->next;
            free(temp);
        } else {
            curr = curr->next;
        }
    }
}
```

**Criterios de aceptación:**
- [ ] Marca el bloque como `BLOCK_FREE` y restablece `process_id = -1`.
- [ ] Fusiona bloques libres contiguos (hacia adelante y hacia atrás en la lista).
- [ ] Emite mensaje informativo indicando cuántos bloques fueron fusionados.

**🔗 Depende de:** Issue 3.3

---

## 🚩 MILESTONE 4: Módulo 3 — Memoria: Cola de Espera por RAM Insuficiente (mem_queue)

> **Módulo:** Módulo 3: Memoria (Gestión de Cola de Procesos)
> **Objetivo:** Implementar una cola de espera FIFO para procesos que no pueden ser asignados a RAM por falta de espacio contiguo (fragmentación) o RAM llena, permitiendo inspección con `mem-queue` y desencolado reactivo al liberar memoria.
> 🏷️ **Etapa Git:** Sub-commit Módulo 3 — `"feat: memory waiting queue"` | ⏱️ Duración estimada: ~3 días

---

### ├─ 📝 Issue 4.1: Estructura de cola FIFO para procesos en espera
**Descripción:**
Crear en `src/memory/mem_queue.h` y `src/memory/mem_queue.c` las estructuras de datos y primitivas de una cola enlazada FIFO (`enqueue`, `dequeue`, `peek`, `clear`).

**Estructuras de referencia:**
```c
#ifndef MEM_QUEUE_H
#define MEM_QUEUE_H

#include <time.h>

typedef struct QueueNode {
    int pid;
    int size_kb;
    time_t enqueued_at;
    struct QueueNode *next;
} QueueNode;

typedef struct {
    QueueNode *front;
    QueueNode *rear;
    int count;
    int total_waiting_kb;
} MemQueue;

extern MemQueue mem_queue;

void mem_queue_init(void);
int  mem_queue_enqueue(int pid, int size_kb);
QueueNode *mem_queue_peek(void);
QueueNode *mem_queue_dequeue(void);
int  mem_queue_remove(int pid);
void mem_queue_clear(void);
void mem_queue_print(void);

#endif
```

**Criterios de aceptación:**
- [ ] Inserción al final (`rear`) y extracción al frente (`front`) en tiempo O(1).
- [ ] Rastreo de `count` (número de procesos) y `total_waiting_kb` acumulado.
- [ ] `mem_queue_clear` libera cada nodo dinámico sin fugas de memoria.

**🔗 Depende de:** Issue 3.1

---

### ├─ 📝 Issue 4.2: Encolado automático ante memoria RAM insuficiente
**Descripción:**
Integrar la cola con la rutina general de asignación (`mem-alloc <pid> <size>`). Si la asignación en First-Fit (`ff_alloc`) o en Paginación (`pag_alloc`) falla por falta de capacidad o fragmentación, el proceso se encola automáticamente en lugar de ser descartado con error terminal.

**Flujo en terminal:**
```
miniOS> mem-alloc 105 512
[ALERTA] Memoria RAM insuficiente para asignar 512 KB al PID 105.
[QUEUE] Proceso PID 105 encolado en cola de espera (Posición: 1).
miniOS> 
```

**Criterios de aceptación:**
- [ ] Si la asignación falla, no aborta ni corrompe la memoria.
- [ ] Registra al proceso en `mem_queue` con su PID, tamaño en KB y timestamp de llegada.
- [ ] Emite notificación clara al usuario indicando su posición en la cola.

**🔗 Depende de:** Issues 3.3 (FF alloc) y 4.1 (Estructuras de cola)

---

### ├─ 📝 Issue 4.3: Comando mem-queue — Inspección de procesos en espera
**Descripción:**
Implementar el comando `mem-queue` en el shell que renderiza la lista de procesos actualmente en espera de RAM, mostrando su posición en cola, PID, memoria solicitada y tiempo transcurrido en espera.

**Salida en terminal:**
```
miniOS> mem-queue
╔═════╦════════╦══════════════╦═════════════════════╗
║ Pos ║ PID    ║ Tamaño (KB)  ║ Tiempo en espera    ║
╠═════╬════════╬══════════════╬═════════════════════╣
║  1  ║ 105    ║ 512 KB       ║ 00:01:14            ║
║  2  ║ 108    ║ 256 KB       ║ 00:00:28            ║
╚═════╩════════╩══════════════╩═════════════════════╝
Total procesos en cola: 2 | Memoria requerida: 768 KB
```

**Criterios de aceptación:**
- [ ] Muestra mensaje amigable si la cola está vacía: `[INFO] La cola de espera de RAM está vacía.`
- [ ] Formato tabular limpio con cálculo del tiempo transcurrido desde la inserción.

**🔗 Depende de:** Issue 4.2

---

### ├─ 📝 Issue 4.4: Desencolado reactivo y asignación automática tras liberación
**Descripción:**
Al liberar memoria (`mem-free <pid>` o fin de proceso), tras aplicar la coalescencia (en First-Fit) o liberar marcos (en Paginación), el sistema verifica automáticamente el frente de la cola (`peek`). Si el proceso en espera ahora cabe en la memoria libre, se desencola y se asigna a RAM automáticamente.

**Implementación de referencia:**
```c
void mem_queue_process_waiting(void) {
    while (mem_queue.count > 0) {
        QueueNode *front = mem_queue_peek();
        int allocated = 0;
        if (strcmp(mem_state.algorithm, "ff") == 0) {
            allocated = (ff_alloc(front->pid, front->size_kb) == 0);
        } else {
            allocated = (pag_alloc(front->pid, front->size_kb) == 0);
        }

        if (allocated) {
            printf("[AUTO-ALLOC] Proceso en espera PID %d asignado exitosamente en RAM (%d KB).\n",
                   front->pid, front->size_kb);
            QueueNode *node = mem_queue_dequeue();
            free(node);
        } else {
            // El proceso al frente aún no cabe; mantener orden FIFO
            break;
        }
    }
}
```

**Criterios de aceptación:**
- [ ] Se invoca automáticamente al finalizar `mem-free`.
- [ ] Asigna tantos procesos de la cola como quepan en la memoria disponible respetando el orden FIFO.
- [ ] Muestra notificación en consola por cada proceso promovido de la cola a la RAM.

**🔗 Depende de:** Issues 3.4 (FF free) y 4.3 (mem-queue)

---

## 🚩 MILESTONE 5: Módulo 3 — Memoria: Paginación y Traducción de Direcciones

> **Módulo:** Módulo 3: Memoria (Simulación de Paginación)
> **Objetivo:** Implementar la asignación de marcos físicos no contiguos, construcción de tablas de páginas por proceso, visualizador con `mem-page-table` y traducción matemática de direcciones lógicas a físicas (`mem-translate`).
> 🏷️ **Etapa Git:** Sub-commit Módulo 3 — `"feat: memory paging translation"` | ⏱️ Duración estimada: ~4 días

---

### ├─ 📝 Issue 5.1: Estructuras de paginación y asignación de marcos
**Descripción:**
Definir en `src/memory/paging.h` la tabla de marcos globales (`FrameTable`) y las tablas de páginas de cada proceso (`PageTable`). Implementar `pag_alloc(pid, size_kb)` que asigna marcos libres sin requerir contigüidad física.

**Estructuras de referencia:**
```c
#define MAX_PAGES 256
#define MAX_PROC  32

typedef struct {
    int frame_number;
    int valid;
} PageEntry;

typedef struct {
    int process_id;
    int num_pages;
    PageEntry pages[MAX_PAGES];
} PageTable;

typedef struct {
    int total_frames;
    int free_frames;
    int frame_owner[MAX_PAGES]; // -1 si libre, PID si ocupado
} FrameTable;

extern FrameTable frame_table;
extern PageTable  page_tables[MAX_PROC];
```

**Criterios de aceptación:**
- [ ] Calcula el número de páginas requeridas con división techo: `(size_kb + page_size - 1) / page_size`.
- [ ] Si los marcos libres son insuficientes, recurre a la cola de espera (`mem_queue`).
- [ ] Asigna marcos libres dispersos (no contiguos) registrando el mapeo en la tabla del proceso.

**🔗 Depende de:** Issues 3.2 (Configuración de marcos y páginas) y 4.1 (mem_queue)

---

### ├─ 📝 Issue 5.2: Traducción de dirección lógica a física (mem-translate)
**Descripción:**
Implementar el comando `mem-translate <pid> <dir_logica>` aplicando la formulación teórica de paginación para calcular número de página y desplazamiento (*offset*), validando límites de direccionamiento.

**Formulación matemática:**
```
num_pagina   = dir_logica / page_size_kb
offset       = dir_logica % page_size_kb
marco_fisico = PageTable[pid].pages[num_pagina].frame_number
dir_fisica   = (marco_fisico * page_size_kb) + offset
```

**Salida en consola:**
```
miniOS> mem-translate 42 192
[PAG] PID 42 | Dir. Lógica: 192 KB -> Dir. Física: 704 KB
      (Página: 3, Marco Físico: 10, Desplazamiento: 0 KB)
```

**Criterios de aceptación:**
- [ ] Verifica que el algoritmo activo sea `pag`; si está en `ff`, rechaza con mensaje informativo indicando usar `switch-ram pag`.
- [ ] Valida que el proceso exista y la página esté dentro de su límite (`valid == 1`).
- [ ] Detecta fallos de dirección inválida con mensaje: `[ERROR] Fallo de segmentación / página no asignada`.

**🔗 Depende de:** Issue 5.1

---

### ├─ 📝 Issue 5.3: Visualizador de tabla de páginas y liberación de marcos
**Descripción:**
Implementar el comando `mem-page-table <pid>` para visualizar la tabla de páginas del proceso y `pag_free(pid)` para liberar los marcos ocupados devolviéndolos al pool global de marcos libres.

**Salida esperada:**
```
miniOS> mem-page-table 42
╔══════════╦════════════╦══════════╦══════════════════╗
║ Página # ║ Marco Fís. ║ Válido   ║ Dir. Base Física ║
╠══════════╬════════════╬══════════╬══════════════════╣
║    0     ║     3      ║    SÍ    ║   192 KB         ║
║    1     ║    10      ║    SÍ    ║   640 KB         ║
║    2     ║     7      ║    SÍ    ║   448 KB         ║
╚══════════╩════════════╩══════════╩══════════════════╝
```

**Criterios de aceptación:**
- [ ] Verifica que el algoritmo activo sea `pag`; en modo `ff` rechaza la ejecución indicando la incompatibilidad.
- [ ] Imprime la relación página -> marco con formato amigable.
- [ ] `pag_free` libera los marcos en `FrameTable` e invoca `mem_queue_process_waiting` para asignar procesos en espera.

**🔗 Depende de:** Issue 5.2

---

## 🚩 MILESTONE 6: Módulo 3 — Memoria: Visualizador Terminal (mem-view) y Switch Seguro

> **Módulo:** Módulo 3: Memoria (Visualización Integral y Conmutación)
> **Objetivo:** Implementar el visualizador interactivo de RAM en terminal (`mem-view` / `ram-view`) mostrando el mapa de memoria y la cola de espera, junto con el comando `switch-ram [ff|pag]` dotado de confirmación de seguridad interactiva.
> 🏷️ **Etapa Git:** Sub-commit Módulo 3 — `"feat: memory visualizer and switch-ram"` | ⏱️ Duración estimada: ~4 días

---

### ├─ 📝 Issue 6.1: Visualizador gráfico interactivo de RAM en terminal (mem-view)
**Descripción:**
Desarrollar el comando `mem-view` (o alias `ram-view`) que renderiza en terminal la disponibilidad de RAM en tiempo real, adaptando la representación gráfica al algoritmo activo (First-Fit o Paginación) e incluyendo el estado de la cola de espera.

**Renderizado en modo First-Fit:**
```
miniOS> mem-view
╔════════════════════════════════════════════════════════════════════╗
║                      MAPA DE MEMORIA RAM (MiniOS)                  ║
║                      Algoritmo Activo: FIRST-FIT                   ║
╠════════════════════════════════════════════════════════════════════╣
║  [P10: 256K | USED] [FREE: 128K] [P20: 384K | USED] [FREE: 256K]   ║
╠════════════════════════════════════════════════════════════════════╣
║  Total: 1024 KB | Usado: 640 KB (62.5%) | Libre: 384 KB (37.5%)    ║
║  Fragmentación Externa: 2 bloques libres no contiguos (384 KB)     ║
║  Cola de Espera RAM: 1 proceso(s) en espera (PID 105: 512 KB)      ║
╚════════════════════════════════════════════════════════════════════╝
```

**Renderizado en modo Paginación:**
```
miniOS> mem-view
╔════════════════════════════════════════════════════════════════════╗
║                      MAPA DE MEMORIA RAM (MiniOS)                  ║
║                      Algoritmo Activo: PAGINACIÓN                  ║
╠════════════════════════════════════════════════════════════════════╣
║  Marcos Físicos (Total: 16 marcos de 64 KB c/u):                   ║
║  [M00: P10] [M01: P10] [M02: P10] [M03: P10]                      ║
║  [M04: P20] [M05: P20] [M06: LIBRE] [M07: LIBRE]                  ║
║  [M08: LIBRE] [M09: LIBRE] [M10: LIBRE] [M11: LIBRE]              ║
║  [M12: LIBRE] [M13: LIBRE] [M14: LIBRE] [M15: LIBRE]              ║
╠════════════════════════════════════════════════════════════════════╣
║  Total: 1024 KB | Usado: 384 KB (6 marcos) | Libre: 640 KB (10 m) ║
║  Cola de Espera RAM: Vacía (0 procesos esperando)                 ║
╚════════════════════════════════════════════════════════════════════╝
```

**Criterios de aceptación:**
- [ ] En First-Fit grafica la secuencia de bloques ocupados y libres con cálculo de fragmentación.
- [ ] En Paginación grafica la matriz de marcos físicos indicando el PID o estado LIBRE.
- [ ] Muestra siempre el estado actual de la cola de espera (`mem_queue`).

**🔗 Depende de:** Issues 3.4 (FF), 4.3 (Queue), 5.3 (Paging)

---

### ├─ 📝 Issue 6.2: Comando switch-ram con capa de seguridad interactiva
**Descripción:**
Implementar `switch-ram [ff|pag]` para alternar en caliente el algoritmo de gestión de memoria. Si no hay procesos en RAM ni en cola, conmuta de inmediato. Si existen procesos activos o en espera, solicita confirmación interactiva al usuario antes de proceder.

**Lógica de decisión y confirmación:**
```c
void cmd_switch_ram(const char *target_algo) {
    if (strcmp(target_algo, "ff") != 0 && strcmp(target_algo, "pag") != 0) {
        fprintf(stderr, "Uso: switch-ram [ff|pag]\n");
        return;
    }
    if (strcmp(mem_state.algorithm, target_algo) == 0) {
        printf("[INFO] El sistema ya está operando con el algoritmo: %s\n", target_algo);
        return;
    }

    int active_procs = count_allocated_processes();
    int waiting_procs = mem_queue.count;

    if (active_procs > 0 || waiting_procs > 0) {
        printf("ADVERTENCIA: Hay %d proceso(s) en RAM y %d en cola de espera.\n",
               active_procs, waiting_procs);
        printf("El cambio a '%s' liberará toda la memoria y purgará la cola.\n", target_algo);
        printf("¿Está seguro de continuar? [s/N]: ");
        fflush(stdout);

        char resp[16];
        if (fgets(resp, sizeof(resp), stdin) == NULL || (resp[0] != 's' && resp[0] != 'S')) {
            printf("[INFO] Operación cancelada por el usuario. No se realizaron cambios.\n");
            return;
        }
        // Usuario confirmó: liberar todo
        release_all_memory();
        mem_queue_clear();
        printf("[OK] Memoria y cola purgadas.\n");
    }

    // Conmutar algoritmo
    strncpy(mem_state.algorithm, target_algo, sizeof(mem_state.algorithm) - 1);
    reset_memory_structures();
    printf("[OK] Algoritmo cambiado exitosamente a: %s\n", 
           strcmp(target_algo, "ff") == 0 ? "First-Fit" : "Paginación");
}
```

**Criterios de aceptación:**
- [ ] Memoria libre y cola vacía: cambia directamente sin interrupciones.
- [ ] Memoria o cola con procesos: lanza advertencia y confirmación interactiva `[s/N]`.
- [ ] Si responde 'N' o cancela con Enter, mantiene intactos los procesos y la memoria.
- [ ] Si confirma con 's' o 'S', purga la RAM, limpia la cola y cambia el algoritmo.

**🔗 Depende de:** Issues 3.4, 4.4, 5.3

---

### ├─ 📝 Issue 6.3: Transición atómica de estructuras y reset limpio
**Descripción:**
Asegurar que al ejecutar `switch-ram` la transición no genere memory leaks ni punteros colgantes. Las estructuras de la lista enlazada de First-Fit se reinicializan a un único bloque libre y la tabla de marcos de Paginación queda con todos sus marcos libres.

**Criterios de aceptación:**
- [ ] Cero pérdidas de memoria en la conmutación verificado con Valgrind.
- [ ] `mem-view` refleja el nuevo mapa limpio inmediatamente después del switch.

**🔗 Depende de:** Issue 6.2

---

## 🚩 MILESTONE 7: Módulo 2 — Hilos: Productor-Consumidor con pthread

> **Módulo:** Módulo 2: Hilos (Concurrencia y Sincronización)
> **Objetivo:** Resolver el problema clásico de sincronización entre hilos dentro del mismo proceso utilizando un búfer circular compartido protegido con `pthread_mutex_t` y `pthread_cond_t`, con mínimo 2 productores y 2 consumidores concurrentes.
> 🏷️ **Etapa Git:** Commit 4 — `"feat: module-2 threads producer-consumer"` | ⏱️ Duración estimada: ~4 días

---

### ├─ 📝 Issue 7.1: Búfer circular compartido thread-safe
**Descripción:**
Implementar en `src/threads/buffer.h` y `buffer.c` un búfer circular sincronizado con mutex y dos variables de condición (`not_full` y `not_empty`), garantizando que no existan condiciones de carrera ni lecturas/escrituras inválidas.

**Estructura:**
```c
#define BUFFER_SIZE 8

typedef struct {
    int data[BUFFER_SIZE];
    int head;                  // Inserción (productores)
    int tail;                  // Extracción (consumidores)
    int count;                 // Elementos actuales
    pthread_mutex_t mutex;
    pthread_cond_t  not_full;  // Espera cuando count == BUFFER_SIZE
    pthread_cond_t  not_empty; // Espera cuando count == 0
    int stop_flag;
} SharedBuffer;

void buffer_init(SharedBuffer *b);
void buffer_destroy(SharedBuffer *b);
int  buffer_put(SharedBuffer *b, int item);
int  buffer_get(SharedBuffer *b, int *item);
```

**Criterios de aceptación:**
- [ ] Todo acceso al búfer está sincronizado con el mutex.
- [ ] `pthread_cond_wait` previene el busy-waiting.
- [ ] `buffer_put` y `buffer_get` devuelven estado limpio al ordenar parada.

**🔗 Depende de:** Issue 1.4

---

### ├─ 📝 Issue 7.2: Implementación de hilos productores y consumidores
**Descripción:**
Implementar las funciones de hilo `producer_worker` y `consumer_worker`. Los productores generan números enteros aleatorios y los colocan en el búfer; los consumidores extraen y procesan datos.

**Comportamiento concurrente:**
- Mínimo 2 hilos productores y 2 hilos consumidores corriendo en simultáneo.
- Salida en consola identificando el ID de hilo, ítem generado/consumido y ocupación del búfer.

**Criterios de aceptación:**
- [ ] Ningún ítem se produce si el búfer está lleno (productor se bloquea).
- [ ] Ningún ítem se consume si el búfer está vacío (consumidor se bloquea).
- [ ] Salida en tiempo real limpia y legible en terminal.

**🔗 Depende de:** Issue 7.1

---

### ├─ 📝 Issue 7.3: Comando thread-demo — Demostración concurrente
**Descripción:**
Implementar el comando `thread-demo [p] [c] [n]` para lanzar la ejecución concurrente, configurable con número de productores, consumidores e ítems por productor (con valores por defecto: 2 productores, 2 consumidores, 10 ítems).

**Sintaxis y llamada:**
```
miniOS> thread-demo 2 2 10
miniOS> thread-demo           # Ejecuta con parámetros default
```

**Criterios de aceptación:**
- [ ] Utiliza `pthread_create()` para lanzar hilos y `pthread_join()` para sincronizar la finalización.
- [ ] Detención ordenada mediante broadcast en las variables de condición.

**🔗 Depende de:** Issue 7.2

---

### ├─ 📝 Issue 7.4: Comando thread-stats — Integridad y 0 pérdidas de datos
**Descripción:**
Implementar el comando `thread-stats` que valida la métrica fundamental de sincronización: **Total de ítems producidos == Total de ítems consumidos** (Pérdida de datos = 0).

**Salida en consola:**
```
miniOS> thread-stats
╔══════════════════════════════════════════════════════╗
║           ESTADÍSTICAS — Sesión de Hilos             ║
╠══════════════════════════════════════════════════════╣
║  Productores        : 2                              ║
║  Consumidores       : 2                              ║
║  Ítems producidos   : 20                             ║
║  Ítems consumidos   : 20                             ║
║  Pérdida de datos   : 0  [OK - NINGUNA]              ║
║  Capacidad del búfer: 8 slots                        ║
║  Tiempo de cómputo  : 2.154 segundos                 ║
╚══════════════════════════════════════════════════════╝
```

**Criterios de aceptación:**
- [ ] Verifica que `producidos == consumidos`.
- [ ] Mide el tiempo de ejecución con `clock_gettime(CLOCK_MONOTONIC)`.

**🔗 Depende de:** Issue 7.3

---

## 🚩 MILESTONE 8: Integración Global, Manejo de Señales y Validación de Robustez

> **Módulo:** Sistema / Integración y Calidad
> **Objetivo:** Integrar los 3 módulos, incorporar manejo de señales POSIX (`SIGCHLD` para evitar zombies, `SIGINT` para evitar que Ctrl+C mate el shell), validación estricta de argumentos y pruebas con Valgrind y ThreadSanitizer.
> 🏷️ **Etapa Git:** Sub-commit Integración — `"test: system integration and leaks check"` | ⏱️ Duración estimada: ~3 días

---

### ├─ 📝 Issue 8.1: Manejadores de señales del sistema con sigaction
**Descripción:**
Configurar en `src/process/signals.c` manejadores seguros:
- `SIGCHLD`: Cosecha automática de procesos hijos terminados en background con `waitpid(-1, &status, WNOHANG)` evitando procesos zombies.
- `SIGINT` (`Ctrl+C`): Evita que el shell se cierre; en su lugar imprime una nueva línea con el prompt interactivo `miniOS> `.

**Criterios de aceptación:**
- [ ] Procesos terminados en background no quedan en estado `<defunct>`.
- [ ] Presionar `Ctrl+C` en el prompt no mata el shell principal.

**🔗 Depende de:** Issues 2.3 y 1.2

---

### ├─ 📝 Issue 8.2: Validación estricta de entradas y mensajes de error
**Descripción:**
Revisar la totalidad de comandos para asegurar que entradas malformadas, números negativos o argumentos faltantes no provoquen segmentación o caídas del shell.

**Tabla de validaciones obligatorias:**
| Entrada errónea | Respuesta del sistema |
|:---|:---|
| `mem-alloc` (sin args) | `Uso: mem-alloc <pid> <size_kb>` |
| `mem-alloc -5 200` | `[ERROR] El PID debe ser un entero positivo.` |
| `mem-config page 50` | `[ERROR] El tamaño de página debe ser potencia de 2.` |
| `switch-ram invalid` | `Uso: switch-ram [ff\|pag]` |
| `thread-demo 0 2 10` | `[ERROR] Se requieren mínimo 2 productores y 2 consumidores.` |

**Criterios de aceptación:**
- [ ] Ningún comando genera `Segmentation Fault` ante argumentos arbitrarios.
- [ ] Mensajes de uso claros y descriptivos.

**🔗 Depende de:** Todos los módulos previos (M2, M6, M7)

---

### ├─ 📝 Issue 8.3: Batería de pruebas integradas y casos de prueba
**Descripción:**
Ejecutar los casos de prueba de `docs/use-cases/` que cubren el ciclo completo:
- Shell REPL y comandos built-in.
- Ejecución foreground y background de procesos externos.
- Asignación First-Fit, fragmentación externa y encolado en `mem_queue`.
- Desencolado reactivo de `mem_queue` tras `mem-free`.
- Switch seguro con confirmación `[s/N]` hacia Paginación.
- Traducción de direcciones y detección de accesos inválidos.
- Sesión concurrente de hilos verificando 0 pérdidas.

**Criterios de aceptación:**
- [ ] 100% de los casos de uso documentados pasan satisfactoriamente.

**🔗 Depende de:** Issue 8.2

---

### ├─ 📝 Issue 8.4: Verificación dinámica de fugas con Valgrind y Helgrind
**Descripción:**
Compilar y ejecutar el proyecto bajo **Valgrind** y **ThreadSanitizer / Helgrind** para certificar:
1. Cero bytes perdidos en memoria (`definitely lost: 0 bytes`).
2. Ausencia de condiciones de carrera en los hilos.

**Comando de verificación:**
```bash
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./mini-os
```

**Criterios de aceptación:**
- [ ] Reporte de Valgrind limpio sin fugas de memoria al salir del shell.
- [ ] Reporte guardado en `docs/valgrind_report.txt`.

**🔗 Depende de:** Issue 8.3

---

## 🚩 MILESTONE 9: Entregables Académicos Finales, Documentación y Video Demostrativo

> **Módulo:** Documentación y Entregables Finales
> **Objetivo:** Completar todos los entregables académicos del proyecto: README en GitHub, informe técnico formal (6-10 págs.) y video demostrativo guiado (<= 8 min).
> 🏷️ **Etapa Git:** Commit 5 — `"docs: final deliverables and release"` | ⏱️ Duración estimada: ~3 días

---

### ├─ 📝 Issue 9.1: README.md integral y guía de usuario
**Descripción:**
Actualizar el `README.md` del repositorio con la documentación completa del sistema: descripción del proyecto, requisitos de Linux, guía de compilación (`make`), tabla exhaustiva de comandos con ejemplos, y capturas ilustrativas.

**Criterios de aceptación:**
- [ ] Documentación clara de todas las features implementadas.
- [ ] Instrucciones de compilación reproducibles en Ubuntu/Debian.

**🔗 Depende de:** Issue 8.4

---

### ├─ 📝 Issue 9.2: Informe técnico formal (6 a 10 páginas)
**Descripción:**
Redactar el informe técnico del proyecto cubriendo:
1. Arquitectura modular del sistema MiniOS.
2. Módulo 1: Decisiones de diseño en la gestión de procesos (`fork`, `exec`, `waitpid`, background).
3. Módulo 2: Sincronización de hilos, análisis de condiciones de carrera y garantía de 0 pérdida.
4. Módulo 3: Análisis comparativo entre First-Fit y Paginación, mitigación de fragmentación, diseño de la cola de espera de RAM (`mem_queue`) y capa de seguridad de `switch-ram`.
5. Historial de Git con evidencia de los commits incrementales.
6. Conclusiones y referencias a estándares POSIX.

**Criterios de aceptación:**
- [ ] Extensión de 6 a 10 páginas con código, tablas y diagramas técnicos.
- [ ] Justificación teórica de cada componente desarrollado.

**🔗 Depende de:** Issue 9.1

---

### ├─ 📝 Issue 9.3: Video demostrativo guiado (<= 8 minutos)
**Descripción:**
Grabar la demostración en consola Linux real, cubriendo todas las funcionalidades en el tiempo estipulado:
- 00:00 - 01:00: Presentación y arquitectura general.
- 01:00 - 02:30: Módulo 1 (Comandos, fork/exec, background `&`, `ps-mini`).
- 02:30 - 05:00: Módulo 3 (Configuración `ram`/`page`/`default`, First-Fit, saturación y encolado en `mem_queue`, liberación y auto-asignación, `mem-view`, `switch-ram` con confirmación segura, paginación y `mem-translate`).
- 05:00 - 06:45: Módulo 2 (Concurrencia con `thread-demo`, salida en tiempo real, `thread-stats` con 0 pérdidas).
- 06:45 - 08:00: Historial de commits en Git (`git log --oneline`), ejecución de pruebas y conclusiones.

**Criterios de aceptación:**
- [ ] Duración máxima de 8 minutos sin cortes que alteren la demostración.
- [ ] Evidencia en vivo de la compilación y ejecución de cada comando.

**🔗 Depende de:** Issue 9.2

---

## 🗂️ Tabla Maestra de Comandos del Shell MiniOS

| Comando | Módulo | Descripción / Funcionalidad |
|:---|:---:|:---|
| `help` | Shell Base | Lista todos los comandos disponibles agrupados por módulo |
| `clear` | Shell Base | Limpia el terminal mediante secuencias ANSI |
| `exit` | Shell Base | Cierra limpiamente el shell MiniOS |
| `<cmd> [args]` | Módulo 1 | Ejecuta un binario externo en foreground con `fork()` y `execvp()` |
| `<cmd> [args] &` | Módulo 1 | Lanza un proceso en background de forma asíncrona |
| `ps-mini` | Módulo 1 | Tabla de procesos de la sesión (PID, PPID, comando, estado) |
| `mem-config ram <KB>` | Módulo 3 | Configura la capacidad total de la memoria RAM simulada |
| `mem-config page <KB>` | Módulo 3 | Configura el tamaño de cada marco/página (potencia de 2) |
| `mem-config frames <N>` | Módulo 3 | Configura explícitamente el número de marcos |
| `mem-config default` | Módulo 3 | Restaura presets estándar (`ff default marcos`: 1024 KB RAM, 64 KB página, 16 marcos) |
| `mem-status` | Módulo 3 | Resumen de configuración actual de RAM, marcos y algoritmo |
| `mem-alloc <pid> <size>` | Módulo 3 | Asigna memoria contigua (FF) o marcos (PAG). Si no cabe, encola en `mem_queue` |
| `mem-free <pid>` | Módulo 3 | Libera memoria de un proceso y desencola automáticamente a procesos en espera |
| `mem-queue` | Módulo 3 | Lista los procesos en cola de espera por falta de RAM (PID, tamaño, tiempo) |
| `mem-view` / `ram-view` | Módulo 3 | Visualizador interactivo de RAM en terminal (mapa de bloques, grilla y cola) |
| `mem-page-table <pid>` | Módulo 3 | Muestra la tabla de páginas del proceso (páginas a marcos físicos) |
| `mem-translate <pid> <dir>` | Módulo 3 | Traduce dirección lógica a física con cálculo de offset y marco |
| `switch-ram [ff\|pag]` | Módulo 3 | Cambia algoritmo con confirmación de seguridad `[s/N]` si hay procesos activos |
| `thread-demo [p] [c] [n]` | Módulo 2 | Lanza hilos concurrentes de productor-consumidor con búfer circular |
| `thread-stats` | Módulo 2 | Estadísticas de la sesión de hilos certificando 0 pérdidas de datos |

---
*Documento estructurado bajo metodología ágil para Sistemas Operativos — MiniOS*
