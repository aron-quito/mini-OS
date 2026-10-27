# 📁 PROJECT: MiniSO — Shell Interactivo con Gestión de Procesos, Hilos y Memoria

> **Curso:** Sistemas Operativos | **Plataforma:** Linux | **Prompt:** `miniOS> `

---

## 🗺️ Ruta Crítica (Critical Path)

```
[M1] Infraestructura Base (Shell REPL + Parser + Dispatcher)
        │
        ├──────────────────────────┬──────────────────────────┐
        ▼                          ▼                          ▼
[M2] Módulo 1 — Procesos   [M3] Módulo 3 — Memoria    [M5] Módulo 2 — Hilos
  fork/exec/wait              Config + First-Fit          pthread
                                     │
                             [M4] Módulo 3 — Paginación
                                 + switch-ram
        │                          │                          │
        └──────────────────────────┴──────────────────────────┘
                                   │
                          [M6] Integración + Tests
                                   │
                          [M7] Entregables Finales
```

> **Ruta crítica:** M1 → M3 → M4 → M6 → M7
> M2, M3 y M5 pueden desarrollarse en **paralelo** una vez completado M1.

---

## 📊 Resumen de Milestones

| Milestone | Nombre                         | Issues  | Depende de | Etapa Git |
|-----------|--------------------------------|---------|------------|-----------|
| M1        | Infraestructura Base           | 1.1–1.4 | —          | Commit 1  |
| M2        | Módulo 1 — Procesos            | 2.1–2.4 | M1         | Commit 2  |
| M3        | Módulo 3 — FF + Config + View  | 3.1–3.5 | M1         | Commit 3  |
| M4        | Módulo 3 — Paginación + Switch | 4.1–4.5 | M3         | —         |
| M5        | Módulo 2 — Hilos pthread       | 5.1–5.4 | M1         | Commit 4  |
| M6        | Integración y Pruebas          | 6.1–6.4 | M2,M4,M5   | —         |
| M7        | Entregables Finales            | 7.1–7.3 | M6         | Commit 5  |

---

## 🚩 MILESTONE 1: Infraestructura Base — Shell CLI y Repositorio

> **Objetivo:** Establecer el proyecto en Git, compilar el shell mínimo con prompt interactivo,
> parser de comandos y ciclo de lectura-evaluación.
>
> 🏷️ **Etapa Git:** Commit 1 — `"feat: bootstrap shell base"` | ⏱️ Duración: ~3 días

---

### ├─ 📝 Issue 1.1: Inicializar repositorio Git y estructura de carpetas

**Descripción:**
Crear el repositorio en GitHub con la estructura de directorios del proyecto.
Añadir `.gitignore` para C/C++, `README.md` inicial y `Makefile` base.

**Estructura de carpetas propuesta:**
```
mini-OS/
├── src/
│   ├── main.c
│   ├── shell/
│   │   ├── shell.c / shell.h
│   │   └── parser.c / parser.h
│   ├── process/
│   │   ├── process.c / process.h
│   ├── memory/
│   │   ├── memory.c / memory.h
│   │   ├── first_fit.c / first_fit.h
│   │   └── paging.c / paging.h
│   └── threads/
│       ├── buffer.c / buffer.h
│       └── producer_consumer.c
├── docs/
│   ├── PROJECT_MANAGEMENT.md
│   └── use-cases/
├── tests/
├── Makefile
├── .gitignore
└── README.md
```

**Criterios de aceptación:**
- [ ] Repositorio público en GitHub con al menos 1 commit inicial
- [ ] `.gitignore` incluye `*.o`, `*.out`, binarios y temporales
- [ ] `Makefile` con targets: `all`, `clean`, `run`
- [ ] `README.md` con nombre del proyecto y descripción breve

**🔗 Depende de:** — *(Punto de partida del proyecto)*

---

### ├─ 📝 Issue 1.2: Implementar el loop principal del shell (REPL)

**Descripción:**
Implementar el ciclo Read-Eval-Print-Loop en `src/shell/shell.c`. El shell muestra
el prompt `miniOS> `, lee la línea con `fgets()`, y maneja EOF (`Ctrl+D`) y `exit`.

**Llamadas clave:**
```c
while (1) {
    printf("miniOS> ");
    fflush(stdout);
    if (fgets(line, MAX_LINE, stdin) == NULL) break; // EOF = Ctrl+D
    dispatcher(line);
}
```

**Criterios de aceptación:**
- [ ] El prompt `miniOS> ` se muestra correctamente en cada iteración
- [ ] `exit` termina el proceso principal con código 0
- [ ] `Ctrl+D` (EOF) termina el shell limpiamente
- [ ] Líneas vacías son ignoradas sin crash

**🔗 Depende de:** Issue 1.1 (estructura de carpetas y Makefile)

---

### ├─ 📝 Issue 1.3: Implementar parser de comandos (tokenizador)

**Descripción:**
Desarrollar `src/shell/parser.c` con una función que reciba la línea cruda
y retorne un array de tokens `argv[]`. Debe manejar espacios múltiples, tabs y newline.

**Firma de función propuesta:**
```c
// parser.h
#define MAX_ARGS 64
#define MAX_LINE 1024

typedef struct {
    char *argv[MAX_ARGS];
    int   argc;
} Command;

int  parse_command(const char *line, Command *cmd);
void free_command(Command *cmd);
```

**Implementación con `strtok_r` (thread-safe):**
```c
char *saveptr;
char *line_copy = strdup(line);
char *token = strtok_r(line_copy, " \t\n", &saveptr);
while (token && cmd->argc < MAX_ARGS - 1) {
    cmd->argv[cmd->argc++] = strdup(token);
    token = strtok_r(NULL, " \t\n", &saveptr);
}
cmd->argv[cmd->argc] = NULL; // requerido por execvp
```

**Criterios de aceptación:**
- [ ] Tokeniza correctamente con espacios simples y múltiples
- [ ] El último elemento de `argv` siempre es `NULL`
- [ ] `argc == 0` si la línea está vacía
- [ ] Sin memory leaks (verificar con Valgrind)

**🔗 Depende de:** Issue 1.2 (loop principal del shell)

---

### ├─ 📝 Issue 1.4: Dispatcher de comandos internos (built-ins)

**Descripción:**
Crear el despachador central que reciba un `Command` parseado y lo enrute al módulo correcto.

**Comandos built-in iniciales:**

| Comando | Acción |
|---------|--------|
| `exit`  | Terminar el shell con `exit(0)` |
| `help`  | Listar todos los comandos disponibles |
| `clear` | Limpiar pantalla con códigos ANSI |

**Criterios de aceptación:**
- [ ] Diferencia entre built-ins y comandos externos del sistema
- [ ] `help` lista todos los módulos y comandos disponibles
- [ ] Comandos desconocidos muestran: `miniOS: 'xyz': comando no encontrado`
- [ ] Código extensible con tabla de punteros a funciones

**🔗 Depende de:** Issue 1.3 (parser de comandos)

---

## 🚩 MILESTONE 2: Módulo 1 — Gestión de Procesos con fork/exec/wait

> **Objetivo:** Ejecutar programas externos, permitir concurrencia de al menos
> 2 procesos, y mostrar PID, PPID y estado al finalizar.
>
> 🏷️ **Etapa Git:** Commit 2 — `"feat: module-1 process management"` | ⏱️ Duración: ~4 días

---

### ├─ 📝 Issue 2.1: Ejecución de programas externos con fork() + execvp()

**Descripción:**
Implementar `src/process/process.c` para lanzar cualquier programa externo
usando `fork()` + `execvp()`.

**Implementación clave:**
```c
pid_t launch_process(Command *cmd) {
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return -1; }
    if (pid == 0) {
        // Proceso hijo
        execvp(cmd->argv[0], cmd->argv);
        perror("execvp"); // Solo si execvp falla
        exit(EXIT_FAILURE);
    }
    printf("[Proceso lanzado] PID: %d | PPID: %d\n", pid, getpid());
    return pid;
}
```

**Syscalls:** `fork()`, `execvp()`, `getpid()`, `getppid()`

**Criterios de aceptación:**
- [ ] `ls`, `pwd`, `echo`, `grep` y otros comandos del sistema funcionan
- [ ] El shell NO muere si el comando falla (error de execvp manejado)
- [ ] Se muestra PID del proceso hijo y PPID (el shell)

**🔗 Depende de:** Issue 1.4 (dispatcher para enrutar comandos externos)

---

### ├─ 📝 Issue 2.2: Espera y reporte de estado con waitpid()

**Descripción:**
Extender el lanzador para que reporte PID, código de salida y señal tras terminar.

**Implementación:**
```c
void wait_for_process(pid_t pid) {
    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status))
        printf("  [PID %d] FINALIZADO | Código salida: %d\n",
               pid, WEXITSTATUS(status));
    else if (WIFSIGNALED(status))
        printf("  [PID %d] TERMINADO POR SEÑAL: %d (%s)\n",
               pid, WTERMSIG(status), strsignal(WTERMSIG(status)));
}
```

**Macros POSIX:** `WIFEXITED`, `WEXITSTATUS`, `WIFSIGNALED`, `WTERMSIG`, `strsignal`

**Criterios de aceptación:**
- [ ] Muestra PID, código de salida en terminaciones normales
- [ ] Detecta y reporta procesos terminados por señal (SIGKILL, etc.)
- [ ] El shell recupera control inmediatamente tras `waitpid()`

**🔗 Depende de:** Issue 2.1 (fork + execvp)

---

### ├─ 📝 Issue 2.3: Ejecución concurrente de >= 2 procesos (modo background)

**Descripción:**
Implementar soporte para ejecutar procesos en background con `&` al final del comando.
Mantener una tabla de procesos activos.

**Ejemplo de uso:**
```
miniOS> sleep 5 &
[1] PID: 1234 lanzado en background
miniOS> sleep 3 &
[2] PID: 1235 lanzado en background
miniOS> ps-mini
```

**Estructura de registro:**
```c
#define MAX_BG_PROCS 16

typedef struct {
    pid_t pid;
    char  cmd[MAX_LINE];
    int   active;
    int   exit_code;
} BgProcess;

extern BgProcess bg_table[MAX_BG_PROCS];
```

**Syscalls:** `fork()`, `execvp()`, `waitpid()` con `WNOHANG`, señal `SIGCHLD`

**Criterios de aceptación:**
- [ ] El operador `&` lanza el proceso en background
- [ ] El shell recupera el prompt inmediatamente sin bloquear
- [ ] Al menos 2 procesos corren concurrentemente sin interferirse
- [ ] Los procesos terminados son cosechados sin dejar zombies

**🔗 Depende de:** Issue 2.2 (waitpid y reporte de estado)

---

### ├─ 📝 Issue 2.4: Comando ps-mini — Visualizador de procesos del shell

**Descripción:**
Implementar el comando interno `ps-mini` que muestra PID, PPID, comando
y estado de todos los procesos de la sesión.

**Salida esperada:**
```
miniOS> ps-mini
╔════╦════════╦════════╦═══════════════╦══════════════╗
║ #  ║ PID    ║ PPID   ║ Comando       ║ Estado       ║
╠════╬════════╬════════╬═══════════════╬══════════════╣
║ 1  ║ 1234   ║ 1000   ║ sleep 5       ║ RUNNING      ║
║ 2  ║ 1235   ║ 1000   ║ sleep 3       ║ DONE (0)     ║
╚════╩════════╩════════╩═══════════════╩══════════════╝
```

**Criterios de aceptación:**
- [ ] Lista PID, PPID, comando y estado de cada proceso
- [ ] Diferencia entre RUNNING, DONE(código) y KILLED(señal)
- [ ] Funciona sin procesos activos (mensaje amigable)

**🔗 Depende de:** Issue 2.3 (tabla de procesos bg_table)

---

## 🚩 MILESTONE 3: Módulo 3 — Memoria: Configuración, First-Fit y Visualizador

> **Objetivo:** Simulación matemática de memoria con First-Fit, configuración
> de parámetros de RAM y visualizador de memoria en terminal.
>
> 🏷️ **Etapa Git:** Commit 3 — `"feat: module-3 memory first-fit"` | ⏱️ Duración: ~5 días

---

### ├─ 📝 Issue 3.1: Estructura de datos de la memoria simulada

**Descripción:**
Diseñar las estructuras en `src/memory/memory.h` que representan
la RAM simulada como lista enlazada de bloques.

**Estructuras clave:**
```c
// memory.h
#ifndef MEMORY_H
#define MEMORY_H

typedef enum { BLOCK_FREE, BLOCK_USED } BlockStatus;

typedef struct MemBlock {
    int           id;
    int           start;         // Dirección de inicio en KB
    int           size;          // Tamaño en KB
    BlockStatus   status;
    int           process_id;   // PID propietario (-1 si libre)
    struct MemBlock *next;
} MemBlock;

typedef struct {
    int      total_kb;
    int      page_size_kb;
    int      num_frames;
    MemBlock *block_list;
    char     algorithm[8];      // "ff" o "pag"
} MemoryState;

extern MemoryState mem_state;

void  mem_init(void);
void  mem_reset(void);

#endif // MEMORY_H
```

**Criterios de aceptación:**
- [ ] Estructuras definidas con guardas `#ifndef/#define/#endif`
- [ ] `mem_state` se puede inicializar y reinicializar limpiamente
- [ ] La lista enlazada soporta operaciones de insert, delete y merge

**🔗 Depende de:** Issue 1.4 (dispatcher para registrar comandos de memoria)

---

### ├─ 📝 Issue 3.2: Comandos de configuración inicial de memoria

**Descripción:**
Implementar los comandos que configuran los parámetros iniciales de la RAM simulada.

**Comandos:**
```
miniOS> mem-config ram 1024      # RAM total = 1024 KB
miniOS> mem-config page 64       # Tamaño de página = 64 KB
miniOS> mem-config frames ff     # Algoritmo = First-Fit (default)
miniOS> mem-status
```

**Salida de mem-status:**
```
RAM total   : 1024 KB
Página size : 64 KB
Marcos      : 16
Algoritmo   : First-Fit
Libre       : 1024 KB (100%)
Usado       : 0 KB (0%)
```

**Validaciones:**
- `page_size` debe ser potencia de 2 y `< total_ram`
- `num_frames = total_ram / page_size`
- Error si se reconfigura con memoria ocupada

**Criterios de aceptación:**
- [ ] Los 3 subcomandos de `mem-config` funcionan correctamente
- [ ] `mem-status` muestra el resumen de configuración completo
- [ ] Validaciones de tamaño correctas con mensajes descriptivos

**🔗 Depende de:** Issue 3.1 (estructuras de datos de memoria)

---

### ├─ 📝 Issue 3.3: Algoritmo First-Fit — Asignación de bloques

**Descripción:**
Implementar el algoritmo First-Fit en `src/memory/first_fit.c`.
El sistema recorre la lista y asigna el primer bloque libre suficiente.

**Función principal:**
```c
int ff_alloc(int process_id, int size_kb) {
    MemBlock *curr = mem_state.block_list;
    while (curr != NULL) {
        if (curr->status == BLOCK_FREE && curr->size >= size_kb) {
            if (curr->size > size_kb) split_block(curr, size_kb);
            curr->status = BLOCK_USED;
            curr->process_id = process_id;
            printf("[FF] PID %d -> bloque en %d KB, tamaño %d KB\n",
                   process_id, curr->start, curr->size);
            return 0;
        }
        curr = curr->next;
    }
    fprintf(stderr, "[FF] ERROR: sin bloque contiguo de %d KB\n", size_kb);
    return -1; // Fragmentación externa
}
```

**Comando de uso:**
```
miniOS> mem-alloc <pid> <size_kb>
```

**Criterios de aceptación:**
- [ ] Asigna el primer bloque libre de tamaño suficiente
- [ ] Divide (split) el bloque sobrante correctamente
- [ ] Reporta error descriptivo cuando hay fragmentación externa
- [ ] El `process_id` se registra en el bloque asignado

**🔗 Depende de:** Issue 3.2 (configuración inicial de memoria)

---

### ├─ 📝 Issue 3.4: Algoritmo First-Fit — Liberación y coalescencia

**Descripción:**
Implementar la liberación de bloques con coalescencia (merging)
de bloques libres adyacentes para combatir la fragmentación.

**Implementación:**
```c
int ff_free(int process_id) {
    MemBlock *curr = mem_state.block_list;
    while (curr != NULL) {
        if (curr->status == BLOCK_USED && curr->process_id == process_id) {
            curr->status = BLOCK_FREE;
            curr->process_id = -1;
            merge_free_blocks();
            printf("[FF] PID %d liberado. Coalescencia aplicada.\n", process_id);
            return 0;
        }
        curr = curr->next;
    }
    fprintf(stderr, "[FF] PID %d no encontrado en memoria.\n", process_id);
    return -1;
}

void merge_free_blocks(void) {
    MemBlock *curr = mem_state.block_list;
    while (curr && curr->next) {
        if (curr->status == BLOCK_FREE && curr->next->status == BLOCK_FREE) {
            curr->size += curr->next->size;
            MemBlock *tmp = curr->next;
            curr->next = tmp->next;
            free(tmp);
        } else curr = curr->next;
    }
}
```

**Comando de uso:**
```
miniOS> mem-free <pid>
```

**Criterios de aceptación:**
- [ ] Marca el bloque como FREE y limpia el `process_id`
- [ ] La coalescencia une bloques libres adyacentes correctamente
- [ ] Reporta fragmentación externa tras cada operación
- [ ] Error si el PID no tiene bloques asignados

**🔗 Depende de:** Issue 3.3 (asignación First-Fit)

---

### ├─ 📝 Issue 3.5: Visualizador de RAM en terminal (mem-view)

**Descripción:**
Implementar el comando `mem-view` que muestra un mapa visual de la RAM
en terminal, diferenciando bloques libres y ocupados.

**Salida esperada:**
```
miniOS> mem-view
╔══════════════════════════════════════════════════════════╗
║                   MAPA DE MEMORIA RAM                    ║
║                   Algoritmo: First-Fit                   ║
╠══════════════════════════════════════════════════════════╣
║  [P42: 256K][P99: 128K][FREE: 128K][P10: 256K][FREE:256K]
╠══════════════════════════════════════════════════════════╣
║  Total: 1024 KB | Usado: 640 KB | Libre: 384 KB          ║
║  Fragmentacion externa: 2 bloques libres no contiguos    ║
╚══════════════════════════════════════════════════════════╝
```

**Criterios de aceptación:**
- [ ] Muestra cada bloque con su PID (o FREE) y tamaño
- [ ] Diferencia visualmente bloques libres y ocupados (color ANSI o etiquetas)
- [ ] Muestra RAM total, KB usados y KB libres
- [ ] Calcula y muestra el número de bloques libres no contiguos

**🔗 Depende de:** Issue 3.4 (lista de bloques estable post-coalescencia)

---

## 🚩 MILESTONE 4: Módulo 3 — Paginación, Traducción y Switch-RAM

> **Objetivo:** Paginación con tabla de páginas, traducción de direcciones
> lógicas a físicas, y comando `switch-ram` con capa de seguridad.
>
> ⏱️ Duración estimada: ~5 días

---

### ├─ 📝 Issue 4.1: Estructuras de paginación y tabla de páginas

**Descripción:**
Diseñar las estructuras para el modo paginación en `src/memory/paging.h`.

**Estructuras:**
```c
// paging.h
#define MAX_PROCESSES 32
#define MAX_PAGES     256

typedef struct {
    int frame_number;  // Marco físico asignado (-1 = no asignado)
    int valid;         // Bit de validez
} PageEntry;

typedef struct {
    int       process_id;
    int       num_pages;
    PageEntry table[MAX_PAGES];
} PageTable;

typedef struct {
    int total_frames;
    int free_frames;
    int frame_used[MAX_PAGES]; // 0=libre, PID=ocupado
} FrameTable;

extern FrameTable frame_table;
extern PageTable  page_tables[MAX_PROCESSES];

void paging_init(void);
void paging_reset(void);
```

**Criterios de aceptación:**
- [ ] Las estructuras representan correctamente el modelo de paginación
- [ ] `FrameTable` rastrea el estado de cada marco físico
- [ ] Las tablas se inicializan/limpian sin memory leaks

**🔗 Depende de:** Issue 3.2 (configuración de RAM define num_frames y page_size)

---

### ├─ 📝 Issue 4.2: Asignación de memoria por paginación

**Descripción:**
Implementar `pag_alloc()` que asigna marcos físicos no contiguos a un proceso.

**Implementación:**
```c
int pag_alloc(int process_id, int size_kb) {
    int num_pages = (size_kb + mem_state.page_size_kb - 1) / mem_state.page_size_kb;
    if (frame_table.free_frames < num_pages) {
        fprintf(stderr, "[PAG] Error: %d marcos requeridos, %d disponibles\n",
                num_pages, frame_table.free_frames);
        return -1;
    }
    PageTable *pt = get_or_create_page_table(process_id);
    int page = 0;
    for (int f = 0; f < frame_table.total_frames && page < num_pages; f++) {
        if (frame_table.frame_used[f] == 0) {
            frame_table.frame_used[f] = process_id;
            pt->table[page].frame_number = f;
            pt->table[page].valid = 1;
            page++;
        }
    }
    frame_table.free_frames -= num_pages;
    pt->num_pages = num_pages;
    printf("[PAG] PID %d: %d páginas -> %d marcos físicos (no contiguos OK)\n",
           process_id, num_pages, num_pages);
    return 0;
}
```

**Criterios de aceptación:**
- [ ] La asignación NO requiere marcos contiguos
- [ ] Se construye la tabla de páginas del proceso correctamente
- [ ] Número de páginas calculado con ceiling division correctamente

**🔗 Depende de:** Issue 4.1 (estructuras de paginación)

---

### ├─ 📝 Issue 4.3: Traducción de direcciones lógicas a físicas

**Descripción:**
Implementar `mem-translate <pid> <dir_logica>` con detección de direcciones inválidas.

**Fórmula de traducción:**
```
num_pagina   = dir_logica / page_size
offset       = dir_logica % page_size
marco_fisico = PageTable[proceso][num_pagina].frame_number
dir_fisica   = (marco_fisico * page_size) + offset
```

**Implementación:**
```c
int translate_address(int pid, int logical_addr) {
    int page_num = logical_addr / mem_state.page_size_kb;
    int offset   = logical_addr % mem_state.page_size_kb;
    PageTable *pt = find_page_table(pid);
    if (!pt || page_num >= pt->num_pages || !pt->table[page_num].valid) {
        fprintf(stderr, "[PAG] DIRECCIÓN INVÁLIDA: 0x%X (página %d no asignada)\n",
                logical_addr, page_num);
        return -1;
    }
    int frame = pt->table[page_num].frame_number;
    int phys  = (frame * mem_state.page_size_kb) + offset;
    printf("[PAG] PID %d | Lógica: %d -> Física: %d (Marco: %d, Offset: %d)\n",
           pid, logical_addr, phys, frame, offset);
    return phys;
}
```

**Ejemplo:**
```
miniOS> mem-translate 42 192
[PAG] PID 42 | Lógica: 192 -> Física: 704 (Marco: 10, Offset: 64)
```

**Criterios de aceptación:**
- [ ] Usa la fórmula correcta con el `page_size` de la configuración actual
- [ ] Detecta y reporta claramente las direcciones inválidas
- [ ] Muestra: dirección lógica, física, marco y offset

**🔗 Depende de:** Issue 4.2 (pag_alloc construye la tabla de páginas)

---

### ├─ 📝 Issue 4.4: Visualizador de tabla de páginas (mem-page-table)

**Descripción:**
Implementar `mem-page-table <pid>` que muestra la tabla de páginas de un proceso.

**Salida esperada:**
```
miniOS> mem-page-table 42
╔══════════╦════════════╦══════════╦══════════════════╗
║ Página # ║ Marco Fís. ║ Válido   ║ Dir. Física Base ║
╠══════════╬════════════╬══════════╬══════════════════╣
║    0     ║     3      ║   SÍ     ║   192 KB         ║
║    1     ║    10      ║   SÍ     ║   640 KB         ║
║    2     ║     7      ║   SÍ     ║   448 KB         ║
╚══════════╩════════════╩══════════╩══════════════════╝
```

**Criterios de aceptación:**
- [ ] Muestra: nro. página, marco físico, bit de validez, dirección física base
- [ ] Dir. física base = `marco * page_size`
- [ ] Error descriptivo si el PID no tiene tabla de páginas

**🔗 Depende de:** Issue 4.2 (pag_alloc construye la tabla)

---

### ├─ 📝 Issue 4.5: Comando switch-ram con capa de seguridad

**Descripción:**
Implementar `switch-ram [ff|pag]` que alterna el algoritmo de memoria
con confirmación de seguridad si hay procesos activos.

**Flujo de decisión:**
```
switch-ram [ff|pag]
       |
       ¿Hay procesos con memoria asignada?
       |
      NO --> Cambiar directamente.
             "[OK] Algoritmo cambiado a [X]"
       |
      SÍ --> "ADVERTENCIA: N proceso(s) con memoria asignada."
             "El cambio liberará TODA la memoria. ¿Continuar? [s/N]: "
             |
            's' --> release_all_memory() + reset_memory_structures()
                    "[OK] Algoritmo cambiado a [X]"
            'N' --> "Operación cancelada."  (sin cambios)
```

**Implementación:**
```c
void cmd_switch_ram(const char *algo) {
    int busy = count_allocated_processes();
    if (busy > 0) {
        printf("ADVERTENCIA: %d proceso(s) con memoria asignada.\n", busy);
        printf("El cambio liberará TODA la memoria. ¿Continuar? [s/N]: ");
        fflush(stdout);
        char ans[4];
        if (fgets(ans, sizeof(ans), stdin) == NULL || (ans[0] != 's' && ans[0] != 'S')) {
            printf("Operación cancelada.\n");
            return;
        }
        release_all_memory();
    }
    strncpy(mem_state.algorithm, algo, 7);
    reset_memory_structures();
    printf("[OK] Algoritmo cambiado a: %s\n",
           strcmp(algo, "ff") == 0 ? "First-Fit" : "Paginación");
}
```

**Criterios de aceptación:**
- [ ] Con memoria libre: cambio inmediato sin preguntar
- [ ] Con memoria ocupada: solicita confirmación `[s/N]`
- [ ] Respuesta 'N' cancela la operación sin modificar nada
- [ ] Tras el cambio, las estructuras del algoritmo anterior se limpian
- [ ] `mem-view` refleja el nuevo algoritmo inmediatamente

**🔗 Depende de:** Issues 3.4 (FF-free) + 4.2 (PAG-alloc) + 3.5 (mem-view)

---

## 🚩 MILESTONE 5: Módulo 2 — Hilos: Productor-Consumidor con pthread

> **Objetivo:** Problema productor-consumidor dentro del mismo proceso,
> con búfer circular, mutex y variables de condición. Mínimo 2 productores y 2 consumidores.
>
> 🏷️ **Etapa Git:** Commit 4 — `"feat: module-2 threads producer-consumer"` | ⏱️ Duración: ~4 días

---

### ├─ 📝 Issue 5.1: Diseño del búfer circular compartido (thread-safe)

**Descripción:**
Implementar en `src/threads/buffer.h` y `buffer.c` el búfer circular
protegido por un `pthread_mutex_t` y dos `pthread_cond_t`.

**Estructura:**
```c
// buffer.h
#define BUFFER_SIZE 8

typedef struct {
    int  data[BUFFER_SIZE];
    int  head;                   // Índice de escritura (productores)
    int  tail;                   // Índice de lectura (consumidores)
    int  count;                  // Elementos actuales en el búfer
    pthread_mutex_t mutex;
    pthread_cond_t  not_full;   // Señal: hay espacio para producir
    pthread_cond_t  not_empty;  // Señal: hay datos para consumir
    int  stop;                  // Flag de parada ordenada
} SharedBuffer;

void buffer_init(SharedBuffer *buf);
void buffer_destroy(SharedBuffer *buf);
int  buffer_put(SharedBuffer *buf, int item);   // Productor: inserta
int  buffer_get(SharedBuffer *buf, int *item);  // Consumidor: extrae
```

**Protocolo de buffer_put:**
```c
int buffer_put(SharedBuffer *buf, int item) {
    pthread_mutex_lock(&buf->mutex);
    while (buf->count == BUFFER_SIZE)
        pthread_cond_wait(&buf->not_full, &buf->mutex);
    buf->data[buf->head] = item;
    buf->head = (buf->head + 1) % BUFFER_SIZE;
    buf->count++;
    pthread_cond_signal(&buf->not_empty);
    pthread_mutex_unlock(&buf->mutex);
    return 0;
}
```

**Criterios de aceptación:**
- [ ] `pthread_mutex_t` protege TODO acceso al búfer (sin race conditions)
- [ ] `not_full`: productores esperan cuando `count == BUFFER_SIZE`
- [ ] `not_empty`: consumidores esperan cuando `count == 0`
- [ ] Sin busy-waiting (`sleep()` solo para retardos de demo, no para sincronización)
- [ ] `pthread_cond_signal` después de cada inserción/extracción

**🔗 Depende de:** Issue 1.4 (dispatcher para el comando `thread-demo`)

---

### ├─ 📝 Issue 5.2: Implementar funciones de productor y consumidor

**Descripción:**
Implementar `producer_func()` y `consumer_func()` como puntos de entrada de hilos.
Mínimo 2 productores y 2 consumidores corriendo simultáneamente.

**Implementación:**
```c
typedef struct {
    int id;
    SharedBuffer *buf;
    int num_items;        // -1 = consumidor infinito hasta señal de parada
    int *produced_count;  // Contador compartido (atómico)
    int *consumed_count;
} ThreadArgs;

void *producer_func(void *arg) {
    ThreadArgs *a = (ThreadArgs *)arg;
    for (int i = 0; i < a->num_items; i++) {
        int item = rand() % 100 + 1;
        buffer_put(a->buf, item);
        __sync_fetch_and_add(a->produced_count, 1);
        printf("[PROD %d] Produjo: %3d | Buffer: %d/%d\n",
               a->id, item, a->buf->count, BUFFER_SIZE);
        usleep(rand() % 200000); // retardo de demo: 0-200ms
    }
    return NULL;
}

void *consumer_func(void *arg) {
    ThreadArgs *a = (ThreadArgs *)arg;
    while (1) {
        int item;
        if (buffer_get(a->buf, &item) == -1) break; // señal de parada
        __sync_fetch_and_add(a->consumed_count, 1);
        printf("[CONS %d] Consumió: %3d | Buffer: %d/%d\n",
               a->id, item, a->buf->count, BUFFER_SIZE);
        usleep(rand() % 300000); // retardo de demo: 0-300ms
    }
    return NULL;
}
```

**Criterios de aceptación:**
- [ ] Mínimo 2 productores + 2 consumidores simultáneos
- [ ] Los productores NO sobreescriben datos no consumidos
- [ ] Los consumidores NO consumen datos inexistentes
- [ ] Salida con: ID del hilo, item, estado del búfer en tiempo real

**🔗 Depende de:** Issue 5.1 (búfer circular thread-safe)

---

### ├─ 📝 Issue 5.3: Comando thread-demo — Lanzar y controlar la demostración

**Descripción:**
Implementar el comando `thread-demo` con parámetros configurables
que lanza, espera y reporta la demo del productor-consumidor.

**Uso:**
```
miniOS> thread-demo [num_prod] [num_cons] [items_por_prod]
miniOS> thread-demo 2 2 10
miniOS> thread-demo        # defaults: 2 prod, 2 cons, 10 items
```

**Flujo:**
```c
void cmd_thread_demo(int np, int nc, int items) {
    SharedBuffer buf;
    buffer_init(&buf);
    int produced = 0, consumed = 0;

    pthread_t prod[np], cons[nc];
    ThreadArgs pa[np], ca[nc];

    for (int i = 0; i < np; i++) {
        pa[i] = (ThreadArgs){i+1, &buf, items, &produced, &consumed};
        pthread_create(&prod[i], NULL, producer_func, &pa[i]);
    }
    for (int i = 0; i < nc; i++) {
        ca[i] = (ThreadArgs){i+1, &buf, -1, &produced, &consumed};
        pthread_create(&cons[i], NULL, consumer_func, &ca[i]);
    }

    // Esperar que productores terminen
    for (int i = 0; i < np; i++) pthread_join(prod[i], NULL);

    // Señalizar parada a consumidores
    pthread_mutex_lock(&buf.mutex);
    buf.stop = 1;
    pthread_cond_broadcast(&buf.not_empty);
    pthread_mutex_unlock(&buf.mutex);

    for (int i = 0; i < nc; i++) pthread_join(cons[i], NULL);
    buffer_destroy(&buf);
    // guardar stats para thread-stats
}
```

**Criterios de aceptación:**
- [ ] Lanza mínimo 2 productores + 2 consumidores (defaults si no hay args)
- [ ] El shell espera con `pthread_join` hasta que todos terminan
- [ ] `total_producido == total_consumido` (cero pérdidas garantizado)
- [ ] Reporte final: ítems producidos, consumidos, tiempo de ejecución

**🔗 Depende de:** Issue 5.2 (funciones productor y consumidor)

---

### ├─ 📝 Issue 5.4: Reporte de estadísticas de hilos (thread-stats)

**Descripción:**
Implementar `thread-stats` que muestra estadísticas detalladas de la última sesión.

**Salida esperada:**
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
║  Tiempo de ejecución: 2.437 segundos                 ║
╚══════════════════════════════════════════════════════╝
```

**Medición de tiempo:**
```c
struct timespec start, end;
clock_gettime(CLOCK_MONOTONIC, &start);
// ... demo ...
clock_gettime(CLOCK_MONOTONIC, &end);
double elapsed = (end.tv_sec - start.tv_sec) +
                 (end.tv_nsec - start.tv_nsec) / 1e9;
```

**Criterios de aceptación:**
- [ ] Muestra `total_producido == total_consumido` (verificación de integridad)
- [ ] Indica explícitamente 0 pérdidas (requisito del enunciado)
- [ ] Tiempo medido con `clock_gettime(CLOCK_MONOTONIC, ...)`
- [ ] Reseteable para una nueva sesión con `thread-demo`

**🔗 Depende de:** Issue 5.3 (thread-demo provee los datos de estadísticas)

---

## 🚩 MILESTONE 6: Integración Final, Pruebas y Hardening

> **Objetivo:** Integrar todos los módulos, pruebas exhaustivas,
> robustez ante entradas inválidas y cero memory leaks.
>
> ⏱️ Duración estimada: ~3 días

---

### ├─ 📝 Issue 6.1: Manejo de señales del sistema (SIGCHLD, SIGINT)

**Descripción:**
Implementar manejadores de señales correctos para el shell.

**Implementación:**
```c
// signal_handler.c
void sigchld_handler(int sig) {
    (void)sig;
    int status; pid_t pid;
    // Loop no-bloqueante para cosechar todos los hijos terminados
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
        update_bg_process_status(pid, status);
}

void sigint_handler(int sig) {
    (void)sig;
    write(STDOUT_FILENO, "\nminiOS> ", 9); // async-signal-safe
}

// En main():
struct sigaction sa_chld = {.sa_handler = sigchld_handler};
sigemptyset(&sa_chld.sa_mask);
sa_chld.sa_flags = SA_RESTART;
sigaction(SIGCHLD, &sa_chld, NULL);

struct sigaction sa_int = {.sa_handler = sigint_handler};
sigemptyset(&sa_int.sa_mask);
sigaction(SIGINT, &sa_int, NULL);
```

**Criterios de aceptación:**
- [ ] `Ctrl+C` NO mata el shell, solo imprime nuevo prompt
- [ ] Procesos background terminados son cosechados sin zombies
- [ ] Uso de `sigaction()` en lugar de `signal()` para mayor portabilidad

**🔗 Depende de:** Issue 2.3 (tabla de procesos bg_table)

---

### ├─ 📝 Issue 6.2: Validación robusta de entradas y manejo de errores

**Descripción:**
Revisar todos los comandos para garantizar que manejan entradas
inválidas sin crashes y con mensajes de error descriptivos.

**Tabla de casos a cubrir:**

| Entrada | Comportamiento esperado |
|---------|------------------------|
| `mem-alloc` sin argumentos | `Error: uso: mem-alloc <pid> <size_kb>` |
| `mem-alloc abc 256` | `Error: PID debe ser un entero positivo` |
| `mem-alloc 42 0` | `Error: tamaño debe ser > 0 KB` |
| `thread-demo -1 2 10` | `Error: num_productores debe ser >= 2` |
| `switch-ram xyz` | `Error: uso: switch-ram [ff\|pag]` |
| `mem-translate 99 500` | `Error: PID 99 no tiene memoria asignada` |
| Comando desconocido | `miniOS: 'xyz': comando no encontrado` |

**Conversión numérica segura:**
```c
char *endptr;
errno = 0;
long val = strtol(argv[1], &endptr, 10);
if (errno != 0 || endptr == argv[1] || *endptr != '\0' || val <= 0) {
    fprintf(stderr, "Error: argumento numérico inválido: '%s'\n", argv[1]);
    return;
}
```

**Criterios de aceptación:**
- [ ] Ningún comando crashea el shell con argumentos inválidos
- [ ] Todos los mensajes de error sugieren el uso correcto
- [ ] Se usa `strtol()` con verificación de `errno` y `endptr`

**🔗 Depende de:** Todos los Issues de implementación (2.x, 3.x, 4.x, 5.x)

---

### ├─ 📝 Issue 6.3: Pruebas funcionales y capturas de pantalla en Linux

**Descripción:**
Ejecutar los casos de prueba de `docs/use-cases/` y capturar evidencia en Linux.

**Pruebas mínimas por módulo:**
- **Shell base:** prompt, exit, help, errores, clear
- **Módulo 1:** `ls -la`, `pwd`, proceso en background con `&`, `ps-mini` con 2+ procesos
- **Módulo 3 FF:** `mem-config`, `mem-alloc`, fragmentación, `mem-free`, coalescencia, `mem-view`
- **Módulo 3 PAG:** `switch-ram pag`, `mem-alloc`, `mem-page-table`, `mem-translate`, dirección inválida
- **switch-ram:** con memoria libre, con memoria ocupada (confirmar y cancelar)
- **Módulo 2:** `thread-demo 2 2 10`, `thread-stats` (verificar 0 pérdidas)

**Criterios de aceptación:**
- [ ] Todos los casos de uso del documento `docs/use-cases/` pasan
- [ ] Capturas de pantalla guardadas en `docs/screenshots/`
- [ ] Sin crashes ni comportamiento indefinido

**🔗 Depende de:** Issue 6.2 (validación robusta como prerequisito)

---

### ├─ 📝 Issue 6.4: Verificación de memory leaks con Valgrind

**Descripción:**
Ejecutar el shell bajo Valgrind y corregir todos los leaks detectados.

**Comando de Valgrind:**
```bash
valgrind --leak-check=full --show-leak-kinds=all \
         --track-origins=yes --verbose \
         --log-file=valgrind_report.txt \
         ./mini-os
```

**Criterios de aceptación:**
- [ ] 0 bytes "definitely lost" al salir del shell
- [ ] 0 accesos inválidos de lectura/escritura
- [ ] Todos los leaks del parser y lista enlazada de memoria corregidos
- [ ] Reporte de Valgrind guardado en `docs/valgrind_report.txt`

**🔗 Depende de:** Issue 6.3 (pruebas funcionales completadas)

---

## 🚩 MILESTONE 7: Entregables Finales — README, Informe Técnico y Video Demo

> **Objetivo:** Completar todos los entregables académicos del proyecto.
>
> 🏷️ **Etapa Git:** Commit 5 — `"docs: final deliverables"` | ⏱️ Duración: ~3 días

---

### ├─ 📝 Issue 7.1: README.md completo en GitHub

**Descripción:**
Actualizar el `README.md` raíz con documentación de usuario completa.

**Secciones obligatorias:**
1. Badge de estado + nombre del proyecto
2. Descripción del proyecto y contexto académico
3. Requisitos del sistema: `Linux`, `gcc`, `make`, `pthreads`, `valgrind`
4. Instrucciones de compilación (`make`) y ejecución (`./mini-os`)
5. Tabla completa de comandos con ejemplos de uso
6. Estructura del repositorio (árbol de carpetas)
7. Capturas de pantalla embebidas de cada módulo

**Criterios de aceptación:**
- [ ] Todos los comandos del sistema están documentados en el README
- [ ] Las instrucciones de compilación funcionan en Ubuntu fresh install
- [ ] Las capturas de pantalla están embebidas y son representativas

**🔗 Depende de:** Issue 6.3 (capturas de pantalla disponibles)

---

### ├─ 📝 Issue 7.2: Informe Técnico (6-10 páginas)

**Descripción:**
Redactar el informe técnico del proyecto.

**Estructura del informe:**
1. **Introducción y objetivos** (~0.5 pág.)
2. **Arquitectura del sistema** — diagrama de módulos (~0.5 pág.)
3. **Módulo 1 — Procesos:** decisiones de diseño con `fork`/`exec`/`wait` (~1.5 pág.)
4. **Módulo 2 — Hilos:** modelo productor-consumidor, análisis de condiciones de carrera (~1.5 pág.)
5. **Módulo 3 — Memoria:** comparativa First-Fit vs Paginación, análisis de fragmentación (~2 pág.)
6. **Evidencia de Git:** log de commits mostrando las 3+ etapas (~0.5 pág.)
7. **Conclusiones y lecciones aprendidas** (~0.5 pág.)
8. **Bibliografía** — man pages y estándar POSIX

**Criterios de aceptación:**
- [ ] Entre 6 y 10 páginas (sin contar bibliografía)
- [ ] Incluye fragmentos de código relevantes con explicación
- [ ] Incluye diagramas: flujo fork/exec, modelo productor-consumidor, mapa de memoria
- [ ] Cita correctamente man pages y el estándar POSIX IEEE 1003.1

**🔗 Depende de:** Issue 7.1 (README como referencia de comandos)

---

### ├─ 📝 Issue 7.3: Video demostrativo (máx. 8 minutos)

**Descripción:**
Grabar el video demostrativo mostrando todas las funcionalidades en Linux real.

**Guión sugerido:**

| Tiempo       | Contenido |
|--------------|-----------|
| 00:00–01:00  | Intro: nombre del proyecto, integrantes, contexto académico |
| 01:00–02:00  | Shell base: prompt, help, clear, exit, manejo de errores |
| 02:00–03:30  | Módulo 1: `ls`, fork/exec, `ps-mini`, 2 procesos en background concurrentes |
| 03:30–05:30  | Módulo 3: `mem-config`, FF alloc/free/view, fragmentación, `switch-ram pag`, pag-alloc, `mem-translate`, `mem-page-table`, dirección inválida |
| 05:30–07:00  | Módulo 2: `thread-demo 2 2 10`, salida en tiempo real, `thread-stats` con 0 pérdidas |
| 07:00–08:00  | `git log --oneline` (3+ etapas evidentes), cierre y conclusiones |

**Criterios de aceptación:**
- [ ] Duración <= 8 minutos
- [ ] Se muestran TODAS las funcionalidades del enunciado
- [ ] El código se compila en vivo o se muestra la compilación
- [ ] Se evidencia el historial de Git con `git log --oneline`

**🔗 Depende de:** Issue 6.3 (el sistema funciona completamente)

---

## 🔧 Stack Tecnológico

| Componente           | Tecnología |
|----------------------|------------|
| Lenguaje             | C11 (`gcc -Wall -Wextra -std=c11 -lpthread`) |
| Build System         | GNU Make |
| Hilos                | POSIX Threads (`pthread.h`, flag `-lpthread`) |
| Procesos             | POSIX: `fork`, `execvp`, `waitpid`, `sigaction` |
| Sincronización       | `pthread_mutex_t`, `pthread_cond_t` |
| Detección de leaks   | Valgrind 3.x |
| Control de versiones | Git + GitHub (mínimo 5 commits con etapas evidentes) |
| Plataforma destino   | Linux — Ubuntu 22.04 / Debian Bookworm recomendado |

---

## 🗂️ Tabla de Comandos del Shell (Referencia rápida)

| Comando | Módulo | Descripción |
|---------|--------|-------------|
| `help` | Base | Lista todos los comandos disponibles |
| `exit` | Base | Termina el shell |
| `clear` | Base | Limpia la pantalla |
| `<cmd> [args]` | M1 | Ejecuta programa externo con fork+exec |
| `<cmd> [args] &` | M1 | Ejecuta en background |
| `ps-mini` | M1 | Muestra tabla de procesos de la sesión |
| `mem-config ram <kb>` | M3 | Configura RAM total |
| `mem-config page <kb>` | M3 | Configura tamaño de página |
| `mem-config frames [ff\|pag]` | M3 | Configura algoritmo por defecto |
| `mem-status` | M3 | Muestra configuración actual de memoria |
| `mem-alloc <pid> <size>` | M3 | Asigna memoria (FF o PAG según modo) |
| `mem-free <pid>` | M3 | Libera memoria de un proceso |
| `mem-view` | M3 | Visualizador gráfico de la RAM |
| `mem-page-table <pid>` | M3-PAG | Tabla de páginas de un proceso |
| `mem-translate <pid> <dir>` | M3-PAG | Traduce dirección lógica a física |
| `switch-ram [ff\|pag]` | M3 | Cambia el algoritmo de memoria |
| `thread-demo [p] [c] [n]` | M2 | Lanza demo productor-consumidor |
| `thread-stats` | M2 | Estadísticas de la última sesión de hilos |

---

*Documento generado para: MiniSO — Evaluación de Producto, Unidad 01*
*Última actualización: 2026-10-26*
