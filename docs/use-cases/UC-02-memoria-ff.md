# Casos de Uso — UC-02: Módulo 3 — Memoria First-Fit

**Módulo:** Módulo 3 — Memoria (Algoritmo First-Fit)
**Estructuras clave:** `MemoryState`, `MemBlock` (lista enlazada)
**Actor:** Usuario de consola Linux

---

## UC-02-01: Configurar parámetros iniciales de memoria

**ID:** UC-02-01
**Nombre:** Configurar RAM total, tamaño de página y algoritmo
**Pre-condición:** El shell está en ejecución. No hay memoria asignada.
**Post-condición:** La memoria simulada está configurada y lista para usar.

### Flujo Principal
```
>> mem-config ram 1024
<< [OK] RAM total configurada: 1024 KB

>> mem-config page 64
<< [OK] Tamaño de página: 64 KB
<< [INFO] Número de marcos calculado: 16 (1024 / 64)

>> mem-config frames ff
<< [OK] Algoritmo por defecto: First-Fit

>> mem-status
<< ╔══════════════════════════════════════╗
<< ║     ESTADO DE MEMORIA — MiniOS       ║
<< ╠══════════════════════════════════════╣
<< ║  RAM total   : 1024 KB               ║
<< ║  Página size : 64 KB                 ║
<< ║  Marcos      : 16                    ║
<< ║  Algoritmo   : First-Fit             ║
<< ║  Libre       : 1024 KB (100.0%)      ║
<< ║  Usado       : 0 KB (0.0%)           ║
<< ╚══════════════════════════════════════╝
<< miniOS>
```

### Flujo Alternativo: tamaño de página inválido (no potencia de 2)
```
>> mem-config page 100
<< [ERROR] El tamaño de página debe ser potencia de 2 (ej: 32, 64, 128, 256).
<< miniOS>
```

### Flujo Alternativo: tamaño de página mayor que RAM
```
>> mem-config page 2048
<< [ERROR] El tamaño de página (2048 KB) no puede ser mayor que la RAM (1024 KB).
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

<< [FF] ERROR: No hay bloque contiguo de 600 KB disponible.
<< [INFO] Memoria libre total: 724 KB | Mayor bloque contiguo libre: 424 KB
<< [INFO] Hay 2 bloque(s) libres no contiguos (fragmentación externa).
<< miniOS>
```

### Criterios de aceptación
- El error especifica que es un problema de **contigüidad**, no de espacio total
- Se muestra el mayor bloque libre disponible para orientar al usuario

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
