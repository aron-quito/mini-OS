# Casos de Uso — UC-06: Módulo 3 — Cola de Espera de Memoria RAM (mem_queue)

**Módulo:** Módulo 3 — Memoria (Gestión de Cola de Procesos)
**Estructuras clave:** `MemQueue`, `QueueNode` (Cola FIFO enlazada)
**Comando:** `mem-queue`
**Actor:** Usuario de consola Linux

---

## UC-06-01: Encolado automático en First-Fit por RAM insuficiente o fragmentación

**ID:** UC-06-01
**Nombre:** Proceso colocado en cola de espera al fallar asignación First-Fit
**Pre-condición:** Memoria configurada (1024 KB RAM). Algoritmo activo = First-Fit (`ff`).
La memoria libre total es de 384 KB, dividida en dos bloques no contiguos de 128 KB y 256 KB.
**Post-condición:** El proceso no es descartado con error fatal; se encola en `mem_queue` y la memoria queda intacta.

### Flujo Principal
```
>> mem-view
<< ╔════════════════════════════════════════════════════════════════════╗
<< ║  [P10: 256K | USED] [FREE: 128K] [P20: 384K | USED] [FREE: 256K]   ║
<< ║  Total: 1024 KB | Usado: 640 KB | Libre: 384 KB                    ║
<< ║  Fragmentación Externa: 2 bloques libres no contiguos (384 KB)     ║
<< ║  Cola de Espera RAM: Vacía (0 procesos esperando)                 ║
<< ╚════════════════════════════════════════════════════════════════════╝
<< miniOS>

>> mem-alloc 105 300

<< [ALERTA] Memoria contigua insuficiente para asignar 300 KB al PID 105.
<< [INFO] Causa: Fragmentación externa (bloque libre mayor: 256 KB).
<< [QUEUE] Proceso PID 105 encolado en espera de memoria RAM (Posición: 1).
<< miniOS>

### Flujo Alternativo: Encolado directo al lanzar proceso en background (e.g., sleep)
```
>> sleep 20 300 &

<< [ALERTA RAM] Memoria contigua insuficiente para ejecutar 'sleep 20' en paralelo (Requerido: 300 KB | Mayor bloque libre: 256 KB).
<< [QUEUE] Proceso 'sleep 20' encolado en espera de memoria RAM (Posición: 1).
<< [INFO] Su ejecución se reanudará automáticamente al liberarse memoria.
<< miniOS>
```

>> mem-queue
<< ╔═════╦════════╦══════════════╦═════════════════════╗
<< ║ Pos ║ PID    ║ Tamaño (KB)  ║ Tiempo en espera    ║
<< ╠═════╬════════╬══════════════╬═════════════════════╣
<< ║  1  ║ 105    ║ 300 KB       ║ 00:00:05            ║
<< ╚═════╩════════╩══════════════╩═════════════════════╝
<< Total procesos en cola: 1 | Memoria requerida: 300 KB
<< miniOS>
```

### Criterios de aceptación
- El sistema no aborta ni corrompe los bloques existentes.
- Notifica al usuario que el proceso ha sido colocado en la cola de espera FIFO.
- El comando `mem-queue` muestra el PID 105, 300 KB solicitados y el temporizador.

---

## UC-06-02: Encolado automático en Paginación por marcos libres insuficientes

**ID:** UC-06-02
**Nombre:** Proceso colocado en cola de espera al agotar marcos físicos disponibles
**Pre-condición:** Memoria configurada en modo Paginación (`pag`). Total marcos = 16 (64 KB c/u).
Marcos ocupados = 14 marcos (896 KB). Marcos libres = 2 marcos (128 KB).
**Post-condición:** El proceso queda registrado en `mem_queue`.

### Flujo Principal
```
>> mem-status
<< Algoritmo: Paginación | Marcos totales: 16 | Marcos libres: 2 (128 KB)

>> mem-alloc 112 256

<< [ALERTA] Marcos físicos insuficientes: 4 requeridos, 2 disponibles.
<< [QUEUE] Proceso PID 112 encolado en espera de memoria RAM (Posición: 1).
<< miniOS>
```

### Criterios de aceptación
- Detecta que se necesitan 4 marcos (`ceil(256 / 64) = 4`) y solo hay 2.
- Encola al proceso registrando su PID y memoria demandada.

---

## UC-06-03: Inspección de cola con mem-queue

**ID:** UC-06-03
**Nombre:** Listar procesos en espera con el comando mem-queue
**Pre-condición:** Existen 2 procesos esperando memoria en `mem_queue`.
**Post-condición:** Se imprime la tabla con el orden de prioridad FIFO.

### Flujo Principal
```
>> mem-queue
<< ╔═════╦════════╦══════════════╦═════════════════════╗
<< ║ Pos ║ PID    ║ Tamaño (KB)  ║ Tiempo en espera    ║
<< ╠═════╬════════╬══════════════╬═════════════════════╣
<< ║  1  ║ 105    ║ 300 KB       ║ 00:01:42            ║
<< ║  2  ║ 112    ║ 256 KB       ║ 00:00:35            ║
<< ╚═════╩════════╩══════════════╩═════════════════════╝
<< Total procesos en cola: 2 | Memoria requerida: 556 KB
<< miniOS>
```

### Flujo Alternativo: Cola vacía
```
>> mem-queue
<< [INFO] La cola de espera de memoria RAM está vacía (0 procesos).
<< miniOS>
```

---

## UC-06-04: Desencolado reactivo y auto-asignación tras liberación de memoria

**ID:** UC-06-04
**Nombre:** Al liberar memoria con mem-free, el proceso en espera se asigna automáticamente
**Pre-condición:** PID 105 (300 KB) está en la posición 1 de la cola.
Se libera el proceso PID 20 (384 KB).
**Post-condición:** Tras la coalescencia, el bloque libre ahora es de 128 + 384 = 512 KB (o suficiente para 300 KB); PID 105 se desencola y se aloja en RAM automáticamente.

### Flujo Principal
```
>> mem-free 20
<< [INFO] Liberando memoria asignada al PID 20...
<< [FF] PID 20 liberado. 384 KB devueltos a la RAM.
<< [FF] Coalescencia aplicada: bloque continuo generado de 512 KB.
<< [QUEUE-TRIGGER] Verificando cola de espera de procesos...
<< [AUTO-ALLOC] Proceso en espera PID 105 desencolado y asignado exitosamente en RAM (300 KB).
<< [INFO] Quedan 212 KB libres en el bloque contiguo.
<< miniOS>

>> mem-queue
<< [INFO] La cola de espera de memoria RAM está vacía (0 procesos).
<< miniOS>

>> mem-view
<< ╔════════════════════════════════════════════════════════════════════╗
<< ║  [P10: 256K | USED] [FREE: 128K] [P105: 300K | USED] [FREE: 84K] [FREE: 256K]
<< ║  Total: 1024 KB | Usado: 556 KB | Libre: 468 KB                    ║
<< ║  Cola de Espera RAM: Vacía (0 procesos esperando)                 ║
<< ╚════════════════════════════════════════════════════════════════════╝
```

### Flujo Alternativo: Desencolado automático al finalizar un proceso en background (sleep)
```
[Proceso PID 4531 (sleep 5) finaliza naturalmente tras 5 segundos]
<< [NOTIFICACIÓN] PID 4531 (sleep 5) finalizado (Código de salida: 0).
<< [RAM AUTO-FREE] 128 KB liberados de PID 4531. Coalescencia aplicada.
<< [QUEUE-TRIGGER] Bloque libre contiguo resultante: 384 KB.
<< [AUTO-LAUNCH] Proceso en espera 'sleep 20' (300 KB) desencolado, asignado a RAM y lanzado en background como PID 4535.
<< miniOS>
```

### Criterios de aceptación
- `mem-free` y la terminación natural de procesos en background (atrapada por `SIGCHLD`) disparan la verificación sin requerir comandos adicionales del usuario.
- El proceso sale de la cola (`dequeue`) solo si la asignación es completamente exitosa.
- Si hay más procesos en la cola, evalúa en orden FIFO si el siguiente también cabe.

---

## UC-06-05: Purgado de cola de espera en switch-ram con confirmación

**ID:** UC-06-05
**Nombre:** El comando switch-ram advierte la existencia de procesos en cola y los purga si se confirma
**Pre-condición:** Hay 1 proceso en RAM y 1 proceso en `mem_queue`.
**Post-condición:** Si el usuario confirma `s`, la cola se purga junto con la RAM. Si dice `N`, la cola se mantiene.

### Flujo con confirmación
```
>> switch-ram pag
<< ADVERTENCIA: Hay 1 proceso(s) en RAM y 1 en cola de espera.
<<              El cambio a 'pag' liberará toda la memoria y purgará la cola.
<< ¿Está seguro de continuar? [s/N]: s
<< [INFO] Liberando procesos activos en RAM...
<< [INFO] Purgando cola de espera (1 proceso cancelado)...
<< [OK] Algoritmo cambiado exitosamente a: Paginación
<< miniOS>

>> mem-queue
<< [INFO] La cola de espera de memoria RAM está vacía (0 procesos).
```

### Criterios de aceptación
- Muestra el conteo exacto de procesos en memoria y procesos en cola.
- Purgado de memoria de nodos con `free()` sin dejar leaks.
