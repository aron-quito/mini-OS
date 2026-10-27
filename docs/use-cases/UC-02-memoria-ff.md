# Casos de Uso — UC-02: Módulo 3 — Memoria First-Fit

**Módulo:** Módulo 3 — Memoria (Algoritmo First-Fit — Particionamiento Dinámico Contiguo)
**Estructuras clave:** `MemoryState`, `MemBlock` (lista enlazada dinámica)
**Actor:** Usuario de consola Linux

---

## UC-02-01: Configurar parámetros iniciales de memoria (First-Fit)

**ID:** UC-02-01
**Nombre:** Configurar RAM total e inicializar bloque gigante First-Fit
**Pre-condición:** El shell está en ejecución. No hay memoria asignada.
**Post-condición:** La memoria física simulada está inicializada como un único bloque continuo libre de tamaño total.

### Flujo Principal
```
>> mem-config ram 1024
<< [OK] RAM total configurada: 1024 KB
<< [INFO] Particionamiento dinámico: 1 bloque libre continuo de 1024 KB inicializado.

>> mem-config default
<< [OK] Presets aplicados: RAM = 1024 KB, Algoritmo = First-Fit (Particionamiento Dinámico Contiguo)

>> mem-status
<< ╔════════════════════════════════════════════════╗
<< ║          ESTADO DE MEMORIA — MiniOS            ║
<< ╠════════════════════════════════════════════════╣
<< ║  RAM total   : 1024 KB                         ║
<< ║  Algoritmo   : First-Fit (Partición Dinámica)  ║
<< ║  Esquema     : Bloques contiguos variables     ║
<< ║  Bloques     : 1 bloque gigante libre          ║
<< ║  Libre       : 1024 KB (100.0%)                ║
<< ║  Usado       : 0 KB (0.0%)                     ║
<< ╚════════════════════════════════════════════════╝
<< miniOS>
```

### Flujo Informativo: Configuración de página latente para futuro modo Paginación
```
>> mem-config page 64
<< [OK] Tamaño de página establecido a: 64 KB (16 marcos calculados para 1024 KB de RAM).
<< [INFO] Modo actual: First-Fit (particionamiento dinámico continuo sin marcos).
<<        Este parámetro solo se activará al cambiar a modo Paginación ('switch-ram pag').
<< miniOS>
```

### Flujo Alternativo: reconfigurar con memoria ocupada
```
>> mem-alloc 10 256
>> mem-config ram 512
<< [ADVERTENCIA] No se puede reconfigurar: hay procesos con memoria asignada.
<< Use 'mem-free <pid>' para liberar memoria primero.
<< miniOS>
```

---

## UC-02-02: Asignar memoria con First-Fit (caso exitoso)

**ID:** UC-02-02
**Nombre:** Asignar bloque de memoria a un proceso usando First-Fit
**Pre-condición:** Memoria configurada (1024 KB, página 64 KB, algoritmo FF). Memoria libre.
**Post-condición:** El proceso tiene un bloque de memoria asignado en la lista.

### Flujo Principal
```
>> mem-alloc 42 256
<< [FF] PID 42 -> bloque asignado en 0 KB, tamaño 256 KB
<< [INFO] Bloque dividido: FREE 768 KB restantes (desde 256 KB)
<< miniOS>

>> mem-alloc 99 128
<< [FF] PID 99 -> bloque asignado en 256 KB, tamaño 128 KB
<< [INFO] Bloque dividido: FREE 640 KB restantes (desde 384 KB)
<< miniOS>

>> mem-alloc 10 256
<< [FF] PID 10 -> bloque asignado en 384 KB, tamaño 256 KB
<< [INFO] Bloque dividido: FREE 384 KB restantes (desde 640 KB)
<< miniOS>
```

### Estado de la lista enlazada tras estas 3 asignaciones:
```
[P42: 0..255 KB | USED] -> [P99: 256..383 KB | USED] ->
[P10: 384..639 KB | USED] -> [FREE: 640..1023 KB | 384 KB]
```

---

## UC-02-03: Asignación fallida por fragmentación externa (First-Fit)

**ID:** UC-02-03
**Nombre:** Intentar asignar más memoria de la disponible en un bloque contiguo
**Pre-condición:** Memoria con fragmentación externa (hay espacio libre total pero no contiguo suficiente).

### Configuración previa del estado:
```
>> mem-alloc 1 300
>> mem-alloc 2 300
>> mem-alloc 3 300
>> mem-free 1
>> mem-free 3
```

### Estado actual (con fragmentación):
```
[FREE: 0..299 KB | 300 KB] -> [P2: 300..599 KB | USED] ->
[FREE: 600..1023 KB | 424 KB] -> [NULL]
Libre total: 724 KB | Libre contiguo máximo: 424 KB
```

### Flujo Principal
```
>> mem-alloc 99 600

<< [FF] ALERTA: No hay bloque contiguo de 600 KB disponible en RAM.
<< [INFO] Memoria libre total: 724 KB | Mayor bloque contiguo libre: 424 KB
<< [INFO] Hay 2 bloque(s) libres no contiguos (fragmentación externa).
<< [QUEUE] Proceso PID 99 encolado en cola de espera de RAM (Posición: 1).
<< miniOS>

>> mem-queue
<< ╔═════╦════════╦══════════════╦═════════════════════╗
<< ║ Pos ║ PID    ║ Tamaño (KB)  ║ Tiempo en espera    ║
<< ╠═════╬════════╬══════════════╬═════════════════════╣
<< ║  1  ║ 99     ║ 600 KB       ║ 00:00:03            ║
<< ╚═════╩════════╩══════════════╩═════════════════════╝
```

### Criterios de aceptación
- El sistema detecta la fragmentación externa sin abortar la ejecución.
- El proceso PID 99 es automáticamente encolado en `mem_queue`.
- Se informa al usuario el mayor bloque contiguo y la posición asignada en cola.

---

## UC-02-04: Liberar memoria y observar coalescencia

**ID:** UC-02-04
**Nombre:** Liberar bloques de memoria y verificar la coalescencia automática
**Pre-condición:** Hay bloques asignados a varios procesos.

### Estado inicial:
```
[P42: 256KB | USED] -> [P99: 128KB | USED] -> [P10: 256KB | USED] -> [FREE: 384KB]
```

### Flujo Principal — liberar P99 (bloque del medio):
```
>> mem-free 99
<< [FF] PID 99 liberado.
<< [INFO] Coalescencia: no hay bloques libres adyacentes para unir.
<< miniOS>
```

### Estado tras liberar P99:
```
[P42: 256KB | USED] -> [FREE: 128KB] -> [P10: 256KB | USED] -> [FREE: 384KB]
```

### Flujo Principal — liberar P42 (adyacente a bloque libre):
```
>> mem-free 42
<< [FF] PID 42 liberado.
<< [INFO] Coalescencia aplicada: bloque libre de 128 KB unido con bloque anterior.
<< [INFO] Nuevo bloque libre: 0 KB a 383 KB (384 KB).
<< miniOS>
```

### Estado tras coalescencia:
```
[FREE: 384KB] -> [P10: 256KB | USED] -> [FREE: 384KB]
```

### Flujo Principal — liberar P10 (entre dos bloques libres):
```
>> mem-free 10
<< [FF] PID 10 liberado.
<< [INFO] Coalescencia aplicada: 3 bloques libres unidos en 1.
<< [INFO] Memoria completamente libre: 1024 KB en 1 bloque contiguo.
<< miniOS>
```

### Estado final:
```
[FREE: 1024KB] -> [NULL]
```

---

## UC-02-05: Visualizador de memoria mem-view (First-Fit)

**ID:** UC-02-05
**Nombre:** Ver el mapa visual de la RAM en modo First-Fit
**Pre-condición:** Hay bloques asignados y libres en la memoria simulada.

### Estado de memoria:
```
[P42: 256KB] -> [P99: 128KB] -> [FREE: 128KB] -> [P10: 256KB] -> [FREE: 256KB]
```

### Flujo Principal
```
>> mem-view

<< ╔══════════════════════════════════════════════════════════════════╗
<< ║                    MAPA DE MEMORIA RAM                           ║
<< ║                    Algoritmo: First-Fit | Total: 1024 KB         ║
<< ╠══════════════════════════════════════════════════════════════════╣
<< ║  [  P42  ][  P99  ][ FREE  ][  P10  ][ FREE  ]                  ║
<< ║   256 KB   128 KB   128 KB   256 KB   256 KB                    ║
<< ╠══════════════════════════════════════════════════════════════════╣
<< ║  Usado  : 640 KB (62.5%)                                         ║
<< ║  Libre  : 384 KB (37.5%)                                         ║
<< ║  Fragm. externa: 2 bloque(s) libre(s) no contiguos              ║
<< ╚══════════════════════════════════════════════════════════════════╝
<< miniOS>
```

### Flujo Alternativo: memoria completamente libre
```
>> mem-view

<< ╔══════════════════════════════════════════════════════════════════╗
<< ║  [                       FREE                       ]           ║
<< ║                           1024 KB                               ║
<< ║  Libre  : 1024 KB (100%) | Sin fragmentación                    ║
<< ╚══════════════════════════════════════════════════════════════════╝
```

### Flujo Alternativo: memoria completamente llena
```
>> mem-view

<< ║  [P1: 512KB][P2: 256KB][P3: 128KB][P4: 128KB]                   ║
<< ║  Usado  : 1024 KB (100%) | Libre: 0 KB                          ║
<< ║  Advertencia: Memoria al 100% — no se pueden realizar nuevas asignaciones.
```

---

## UC-02-06: Argumentos inválidos en mem-alloc y mem-free

**ID:** UC-02-06
**Nombre:** Verificar manejo de errores en comandos de memoria
**Pre-condición:** El shell está en ejecución con memoria configurada.
**Post-condición:** Se muestra error descriptivo; el estado de memoria no se altera.

### Caso 1: mem-alloc sin argumentos
```
>> mem-alloc
<< [ERROR] Uso: mem-alloc <pid> <size_kb>
<< Ejemplo: mem-alloc 42 256
<< miniOS>
```

### Caso 2: PID no numérico
```
>> mem-alloc abc 256
<< [ERROR] PID inválido: 'abc'. Debe ser un entero positivo.
<< miniOS>
```

### Caso 3: tamaño cero o negativo
```
>> mem-alloc 42 0
<< [ERROR] Tamaño debe ser > 0 KB.
<< miniOS>
```

### Caso 4: PID ya tiene memoria asignada
```
>> mem-alloc 42 256
>> mem-alloc 42 128
<< [ADVERTENCIA] PID 42 ya tiene 256 KB asignados.
<< Use 'mem-free 42' antes de reasignar.
<< miniOS>
```

### Caso 5: mem-free con PID no asignado
```
>> mem-free 999
<< [ERROR] PID 999 no tiene bloques asignados en memoria.
<< miniOS>
```

---

## UC-02-07: Intento de invocar comandos de Paginación en modo First-Fit (Rechazo seguro)

**ID:** UC-02-07
**Nombre:** Bloqueo de comandos de paginación durante el modo First-Fit
**Pre-condición:** El algoritmo activo es First-Fit (`ff`).
**Post-condición:** Los comandos de paginación son rechazados de forma segura con mensaje explicativo; el estado de la memoria contigua permanece intacto.

### Caso 1: Intentar consultar tabla de páginas en First-Fit
```
>> mem-alloc 42 256
>> mem-page-table 42

<< [ERROR] El comando 'mem-page-table' solo está disponible en modo Paginación.
<< [INFO] En First-Fit la memoria se administra como particiones dinámicas contiguas, sin páginas ni marcos.
<< [INFO] Para utilizar el modelo de paginación, cambie de algoritmo con 'switch-ram pag'.
<< miniOS>
```

### Caso 2: Intentar traducir dirección lógica a física con tabla de páginas en First-Fit
```
>> mem-translate 42 100

<< [ERROR] El comando 'mem-translate' requiere el modo Paginación ('pag').
<< [INFO] First-Fit utiliza direccionamiento contiguo directo (offset base en memoria física).
<< [INFO] Use 'switch-ram pag' si desea experimentar con traducción por tablas de páginas.
<< miniOS>
```
