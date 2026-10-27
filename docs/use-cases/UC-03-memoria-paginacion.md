# Casos de Uso — UC-03: Módulo 3 — Memoria por Paginación

**Módulo:** Módulo 3 — Memoria (Algoritmo de Paginación)
**Estructuras clave:** `PageTable`, `FrameTable`, `PageEntry`
**Fórmula principal:** `dir_fisica = (marco * page_size) + offset`
**Actor:** Usuario de consola Linux

---

## UC-03-01: Asignar memoria por paginación (no contigua)

**ID:** UC-03-01
**Nombre:** Asignar páginas a marcos físicos usando paginación
**Pre-condición:** Memoria configurada con `mem-config`. Modo `pag` activo (tras `switch-ram pag`).
**Post-condición:** El proceso tiene `n` páginas asignadas a marcos físicos no contiguos.

### Configuración previa:
```
>> mem-config ram 1024
>> mem-config page 64
>> switch-ram pag
<< [OK] Algoritmo cambiado a: Paginación
<< [INFO] 16 marcos físicos disponibles (1024 KB / 64 KB)
```

### Flujo Principal
```
>> mem-alloc 42 200

<< [PAG] Proceso PID 42 | Tamaño solicitado: 200 KB
<< [PAG] Páginas necesarias: 4 (ceil(200/64) = 4 * 64 = 256 KB reservados)
<< [PAG] Marcos asignados: 0, 1, 2, 3
<< [PAG] PID 42: 4 páginas asignadas a marcos físicos 0,1,2,3
<< miniOS>

>> mem-alloc 99 64

<< [PAG] Proceso PID 99 | Tamaño solicitado: 64 KB
<< [PAG] Páginas necesarias: 1
<< [PAG] Marco asignado: 4
<< [PAG] PID 99: 1 página asignada al marco 4
<< miniOS>

>> mem-alloc 77 128

<< [PAG] Proceso PID 77 | Tamaño solicitado: 128 KB
<< [PAG] Páginas necesarias: 2
<< [PAG] Marcos asignados: 5, 6
<< [PAG] PID 77: 2 páginas asignadas a marcos 5,6
<< miniOS>
```

### Estado de la FrameTable tras las 3 asignaciones:
```
Marco 0  -> PID 42 (página 0)
Marco 1  -> PID 42 (página 1)
Marco 2  -> PID 42 (página 2)
Marco 3  -> PID 42 (página 3)
Marco 4  -> PID 99 (página 0)
Marco 5  -> PID 77 (página 0)
Marco 6  -> PID 77 (página 1)
Marco 7..15 -> LIBRE
```

### Criterios de aceptación
- El número de páginas se calcula con ceiling division
- Los marcos se asignan en orden, pero NO necesitan ser contiguos
- No hay fragmentación externa (cualquier conjunto de marcos libres sirve)

---

## UC-03-02: Ver tabla de páginas con mem-page-table

**ID:** UC-03-02
**Nombre:** Visualizar la tabla de páginas de un proceso específico
**Pre-condición:** El proceso tiene páginas asignadas (tras `mem-alloc` en modo PAG).
**Post-condición:** Se muestra la tabla de páginas con dirección física base calculada.

### Flujo Principal (PID 42 con 4 páginas, page_size=64 KB)
```
>> mem-page-table 42

<< ╔══════════╦════════════╦══════════╦══════════════════╗
<< ║ Página # ║ Marco Fís. ║ Válido   ║ Dir. Física Base ║
<< ╠══════════╬════════════╬══════════╬══════════════════╣
<< ║    0     ║     0      ║   SÍ     ║   0 KB           ║
<< ║    1     ║     1      ║   SÍ     ║   64 KB          ║
<< ║    2     ║     2      ║   SÍ     ║   128 KB         ║
<< ║    3     ║     3      ║   SÍ     ║   192 KB         ║
<< ╚══════════╩════════════╩══════════╩══════════════════╝
<< [INFO] PID 42 | Páginas: 4 | Tamaño: 256 KB (4 * 64 KB)
<< miniOS>
```

### Cálculo de Dir. Física Base:
```
Página 0: marco 0 * 64 = 0 KB
Página 1: marco 1 * 64 = 64 KB
Página 2: marco 2 * 64 = 128 KB
Página 3: marco 3 * 64 = 192 KB
```

### Flujo Alternativo: PID sin páginas asignadas
```
>> mem-page-table 999
<< [ERROR] PID 999 no tiene tabla de páginas. No hay memoria asignada.
<< miniOS>
```

### Caso con marcos no contiguos (escenario realista):
```
[Tras varios alloc/free, los marcos 3, 7, 11 están libres]
>> mem-alloc 55 192
>> mem-page-table 55

<< ║    0     ║     3      ║   SÍ     ║   192 KB         ║
<< ║    1     ║     7      ║   SÍ     ║   448 KB         ║
<< ║    2     ║    11      ║   SÍ     ║   704 KB         ║
[Marcos físicamente no contiguos: 3, 7, 11]
```

---

## UC-03-03: Traducción de dirección lógica a física (caso válido)

**ID:** UC-03-03
**Nombre:** Traducir una dirección lógica a dirección física para un proceso
**Pre-condición:** El proceso tiene una tabla de páginas válida. Modo PAG activo.
**Post-condición:** Se muestra la dirección física correspondiente.

### Configuración: PID 42, page_size=64 KB, marcos 0,1,2,3

### Flujo Principal
```
>> mem-translate 42 0
<< [PAG] PID 42 | Dirección lógica: 0
<< [PAG] Página: 0 (0 / 64) | Offset: 0 (0 % 64)
<< [PAG] Marco físico: 0 | Dir. física base: 0 KB
<< [PAG] Dirección física: 0 KB (0 + 0)
<< miniOS>

>> mem-translate 42 75
<< [PAG] PID 42 | Dirección lógica: 75 KB
<< [PAG] Página: 1 (75 / 64 = 1) | Offset: 11 (75 % 64 = 11)
<< [PAG] Marco físico: 1 | Dir. física base: 64 KB
<< [PAG] Dirección física: 75 KB (64 + 11)
<< miniOS>

>> mem-translate 42 200
<< [PAG] PID 42 | Dirección lógica: 200 KB
<< [PAG] Página: 3 (200 / 64 = 3) | Offset: 8 (200 % 64 = 8)
<< [PAG] Marco físico: 3 | Dir. física base: 192 KB
<< [PAG] Dirección física: 200 KB (192 + 8)
<< miniOS>
```

### Cálculos verificados:
```
Entrada: logical=75, page_size=64
  page_num = 75 / 64 = 1
  offset   = 75 % 64 = 11
  frame    = PageTable[42][1].frame_number = 1
  physical = (1 * 64) + 11 = 75  ← Coincide (porque marco 1 es el primero después del 0)

Entrada: logical=200, page_size=64
  page_num = 200 / 64 = 3
  offset   = 200 % 64 = 8
  frame    = PageTable[42][3].frame_number = 3
  physical = (3 * 64) + 8 = 200
```

---

## UC-03-04: Traducción de dirección lógica inválida

**ID:** UC-03-04
**Nombre:** Detectar y reportar una dirección lógica fuera del espacio del proceso
**Pre-condición:** El proceso PID 42 tiene 4 páginas asignadas (páginas 0-3, hasta 256 KB).

### Caso 1: Página fuera de rango (dirección demasiado grande)
```
>> mem-translate 42 300

<< [PAG] PID 42 | Dirección lógica: 300 KB
<< [PAG] Página calculada: 4 (300 / 64 = 4)
<< [PAG] DIRECCIÓN INVÁLIDA: La página 4 no está asignada al proceso 42.
<< [PAG] Páginas válidas para PID 42: 0 a 3 (0 a 255 KB).
<< miniOS>
```

### Caso 2: PID inexistente
```
>> mem-translate 999 100

<< [PAG] DIRECCIÓN INVÁLIDA: PID 999 no tiene tabla de páginas.
<< miniOS>
```

### Criterios de aceptación
- El sistema NO falla (no crash, no acceso a memoria inválida en C)
- El mensaje indica claramente: qué página se intentó acceder y cuáles son válidas
- El bit de validez `valid=0` es chequeado explícitamente

---

## UC-03-05: Liberar memoria en modo paginación

**ID:** UC-03-05
**Nombre:** Liberar marcos físicos de un proceso en modo PAG
**Pre-condición:** El proceso tiene páginas asignadas en modo paginación.
**Post-condición:** Los marcos se liberan; la tabla de páginas del proceso se invalida.

### Flujo Principal
```
>> mem-alloc 42 256
<< [PAG] PID 42: 4 páginas en marcos 0,1,2,3

>> mem-free 42
<< [PAG] PID 42 liberado.
<< [INFO] Marcos liberados: 0, 1, 2, 3
<< [INFO] Tabla de páginas de PID 42 eliminada.
<< [INFO] Marcos disponibles: 16 / 16
<< miniOS>

>> mem-page-table 42
<< [ERROR] PID 42 no tiene tabla de páginas. No hay memoria asignada.
```

### Verificación con mem-status:
```
>> mem-status
<< Libre: 1024 KB (100%) | Sin fragmentación externa (paginación)
```

---

## UC-03-06: Asignación fallida por marcos insuficientes (PAG)

**ID:** UC-03-06
**Nombre:** Intentar asignar más páginas de las que hay marcos libres
**Pre-condición:** Hay pocos marcos libres disponibles.

### Estado: 14 de 16 marcos usados, solo 2 libres
```
>> mem-alloc 100 200   # Necesita 4 páginas (ceil(200/64)=4), solo hay 2 marcos libres

<< [PAG] ERROR: Marcos insuficientes.
<< [PAG] Páginas requeridas: 4 | Marcos libres disponibles: 2
<< [INFO] No es posible asignar 200 KB a PID 100.
<< miniOS>
```

### Diferencia con First-Fit:
- En PAG no hay fragmentación externa — si hay marcos libres, se pueden usar
- Si hay menos marcos que páginas requeridas, la asignación falla siempre

---

## UC-03-07: Visualizador mem-view en modo Paginación

**ID:** UC-03-07
**Nombre:** Ver el estado visual de los marcos físicos en modo PAG
**Pre-condición:** Modo paginación activo. Varios procesos con páginas asignadas.

### Flujo Principal
```
>> mem-view

<< ╔══════════════════════════════════════════════════════════════════╗
<< ║                   MAPA DE MARCOS FÍSICOS                         ║
<< ║                   Algoritmo: Paginación | 16 marcos x 64 KB      ║
<< ╠══════════════════════════════════════════════════════════════════╣
<< ║ M00[P42p0] M01[P42p1] M02[P42p2] M03[P42p3] M04[P99p0]          ║
<< ║ M05[P77p0] M06[P77p1] M07[ FREE ] M08[ FREE ] M09[ FREE ]        ║
<< ║ M10[ FREE ] M11[ FREE ] M12[ FREE ] M13[ FREE ] M14[ FREE ]       ║
<< ║ M15[ FREE ]                                                        ║
<< ╠══════════════════════════════════════════════════════════════════╣
<< ║  Marcos usados: 7 / 16 | Marcos libres: 9 / 16                  ║
<< ║  Memoria usada: 448 KB | Memoria libre: 576 KB                   ║
<< ║  Sin fragmentación externa (característica de paginación)         ║
<< ╚══════════════════════════════════════════════════════════════════╝
<< miniOS>
```

### Convención de etiquetas:
- `M00[P42p0]` = Marco físico 0, asignado a PID 42, página lógica 0
- `M07[ FREE ]` = Marco 7, libre
