# 📋 Especificación Técnica: SIS-53 — Desencolado Reactivo y Asignación Automática tras Liberación (`mem_queue_process_waiting`) (Issue 4.4)

> **Identificador Linear:** `SIS-53` | **Hito:** `Milestone 4: Cola de Espera de Memoria (FIFO)`  
> **Proyecto / Módulo:** `Módulo 3 — Gestión de Memoria (First-Fit & Paginación)`  
> **Alcance Estricto:** Implementación de la rutina reactiva `mem_queue_process_waiting(void)` en `src/memory/mem_queue.c` y su integración automática en el ciclo de liberación de recursos (`ff_free`, `pag_free` y `memory_free`) preservando el orden estricto FIFO (*Head-of-Line Blocking*).  
> **Estado:** Completado (`Done`) | **Etapa Git:** `feature/modulo3-desencolado`  
> **🔗 Depende de:**  
> - [SIS-49](sis-49.md) (*Issue 3.4: Liberación de memoria con coalescencia en First-Fit `ff_free`*)  
> - `SIS-50` (*Issue 4.1: Estructura de cola FIFO para procesos en espera `mem_queue`*)  
> - `SIS-51` (*Issue 4.2: Encolado automático cuando la RAM es insuficiente*)  
> - `SIS-52` (*Issue 4.3: Inspección de la cola de espera `mem-queue`*)  
> **🔒 Bloquea a:**  
> - [SIS-56](sis-56.md) (*Issue 5.3: Liberación de memoria paginada `pag_free` con promoción reactiva*)  
> - `SIS-57` (*Issue 6.1: Visualizador gráfico interactivo de RAM en terminal `mem-view`*)  

---

## 1. Objetivo y Alcance Estricto de la Tarea

La tarea **SIS-53 (Issue 4.4)** cierra el **Milestone 4** automatizando la promoción de procesos retenidos en la cola de espera de RAM (`mem_queue`) cada vez que se libera memoria en el sistema (ya sea mediante el comando interactivo `mem-free <pid>` o cuando un proceso en segundo plano termina y es cosechado por `process_reap_bg()`).

### 🎯 Lo que SÍ cubre esta tarea:

1. **Inspección FIFO reactiva (`mem_queue_process_waiting(void)`):**
   - Mientras la cola tenga elementos (`q.count > 0`), inspecciona el proceso al frente con `mem_queue_peek()` sin extraerlo todavía.

2. **Intento de asignación según el algoritmo activo (`mem_state.algorithm`):**
   - Si `strcmp(mem_state.algorithm, "ff") == 0`, intenta asignar con `ff_alloc(front->pid, front->size_kb) == 0`.
   - Si `strcmp(mem_state.algorithm, "pag") == 0`, intenta asignar con `pag_alloc(front->pid, front->size_kb) == 0`.

3. **Promoción en cascada o bloqueo (*Head-of-Line Blocking*):**
   - **Éxito (cabe en memoria):**
     - Imprime la notificación de promoción automática:
       ```text
       [AUTO-ALLOC] Proceso en espera PID <pid> asignado exitosamente en RAM (<size_kb> KB).
       ```
     - Extrae el nodo frontal con `mem_queue_dequeue()` y libera su memoria dinámica (`free(node)`).
     - Continúa iterando el bucle `while` para intentar promover al siguiente proceso en la fila si aún queda espacio disponible.
   - **Fallo (no cabe):**
     - Detiene el ciclo inmediatamente (`break`) para preservar el orden estricto **FIFO** (*Head-of-Line Blocking*), evitando que procesos posteriores más pequeños se adelanten al primero de la cola.

4. **Disparo automático tras liberación y coalescencia:**
   - **First-Fit (`ff_free` en `src/memory/first_fit.c`):** Se invoca `mem_queue_process_waiting()` justo después de fusionar bloques libres adyacentes con `merge_adjacent_blocks()`, permitiendo que varios bloques pequeños liberados y coalescidos satisfagan una solicitud grande en espera.
   - **Paginación (`pag_free` en `src/memory/paging.c`):** Se invoca `mem_queue_process_waiting()` tras devolver todos los marcos del proceso a `frame_table`.

---

## 2. Diagrama de Flujo: `mem_queue_process_waiting`

```mermaid
flowchart TD
    FreeTrigger(["Liberación de RAM<br>ff_free(pid) + merge_adjacent_blocks()<br>o pag_free(pid)"]) --> Start(["mem_queue_process_waiting()"])
    Start --> CheckCount{"¿q.count > 0?"}
    CheckCount -- "No (Cola vacía)" --> End(["Fin"])
    CheckCount -- "Sí" --> Peek["front = mem_queue_peek()"]
    Peek --> CheckAlgo{"¿mem_state.algorithm?"}
    CheckAlgo -- '"ff"' --> TryFF["allocated = (ff_alloc(front->pid, front->size_kb) == 0)"]
    CheckAlgo -- '"pag"' --> TryPAG["allocated = (pag_alloc(front->pid, front->size_kb) == 0)"]
    TryFF --> CheckAlloc{"¿allocated == 1?"}
    TryPAG --> CheckAlloc
    CheckAlloc -- "Sí (Cabe en RAM)" --> PrintAuto["Imprimir:<br>[AUTO-ALLOC] Proceso en espera PID &lt;pid&gt;<br>asignado exitosamente en RAM (&lt;size_kb&gt; KB)."]
    PrintAuto --> Deq["node = mem_queue_dequeue()<br>free(node)"]
    Deq --> CheckCount
    CheckAlloc -- "No (Head-of-Line Blocking)" --> Break["break (Preservar orden FIFO estricto)"] --> End
```

---

## 3. Archivos Modificados

| Archivo | Rol en SIS-53 |
|---|---|
| [src/memory/mem_queue.h](file:///Users/luisc/Documents/mini-OS/src/memory/mem_queue.h) | Declaración pública de `void mem_queue_process_waiting(void)`. |
| [src/memory/mem_queue.c](file:///Users/luisc/Documents/mini-OS/src/memory/mem_queue.c) | Implementación canónica de `mem_queue_process_waiting(void)` con soporte dual (`ff` / `pag`) y *Head-of-Line Blocking*. |
| [src/memory/first_fit.c](file:///Users/luisc/Documents/mini-OS/src/memory/first_fit.c) | Llamada automática a `mem_queue_process_waiting()` al final de `ff_free(int pid)`, inmediatamente después de `merge_adjacent_blocks()`. |
| [src/memory/paging.c](file:///Users/luisc/Documents/mini-OS/src/memory/paging.c) | Integración en `pag_free(int pid)` enlazando con la implementación de `mem_queue.c`. |
| [src/memory/memory.c](file:///Users/luisc/Documents/mini-OS/src/memory/memory.c) | Delegación limpia en `memory_free(int pid)` hacia `ff_free` / `paging_free` sin llamadas duplicadas. |

---

## 4. Verificación y Criterios de Aceptación

- ✅ **Respeto estricto de FIFO (*Head-of-Line Blocking*):** Si el primer proceso en `mem_queue` solicita `96 KB`, el segundo solicita `32 KB` y se liberan solo `64 KB`, el ciclo se detiene (`break`) sin promover al segundo proceso por delante del primero.
- ✅ **Coalescencia + Promoción en cascada:** Al liberar un bloque contiguo adicional que fusiona la memoria libre a `128 KB`, `mem_queue_process_waiting()` promueve en una sola llamada tanto al primer proceso (`96 KB`) como al segundo (`32 KB`).
- ✅ **Gestión limpia de memoria dinámica:** Cada nodo promovido se extrae con `mem_queue_dequeue()` y se libera con `free(node)`, manteniendo `q.count` y `q.total_waiting_kb` consistentes y sin fugas de memoria.

