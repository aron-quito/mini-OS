# SIS-65: Validación estricta de entradas y mensajes de error (Issue 8.2)

> **Módulo:** Milestone 8 — Integración Global, Señales y Hardening  
> **Depende de:** Issue 1.4, Issue 2.1, Issue 3.2, Issue 7.3  
> **Estado:** ✅ Completado  
> **Rama:** `feature/issue-8.2-input-validation`

---

## 1. Descripción

Revisar la totalidad de comandos de miniOS para asegurar que entradas malformadas, números negativos o argumentos faltantes no provoquen fallos de segmentación (`Segmentation Fault`) ni caídas abruptas del shell interactivo.

Se incorporan verificaciones exhaustivas de tipos, rangos numéricos y estados previos, y se integran como comandos built-in los comandos de memoria `mem-alloc`, `mem-free`, `switch-ram`, `mem-view`, `mem-page-table` y `mem-translate`.

---

## 2. Tabla de Validaciones Obligatorias y Cumplimiento

| Entrada Errónea | Respuesta del Sistema | Estado |
|:---|:---|:---:|
| `mem-alloc` (sin argumentos) | `[ERROR] Uso: mem-alloc <pid> <size_kb>`<br>`Ejemplo: mem-alloc 42 256` | ✅ Aprobado |
| `mem-alloc -5 200` | `[ERROR] El PID debe ser un entero positivo.` | ✅ Aprobado |
| `mem-alloc 42 0` | `[ERROR] Tamano debe ser > 0 KB.` | ✅ Aprobado |
| `mem-config page 50` | `[ERROR] El tamano de pagina debe ser potencia de 2.` | ✅ Aprobado |
| `switch-ram invalid` | `[ERROR] Algoritmo desconocido: 'invalid'.`<br>`Uso: switch-ram [ff\|pag]` | ✅ Aprobado |
| `thread-demo 0 2 10` | `[ERROR] Se requieren minimo 2 productores y 2 consumidores.` | ✅ Aprobado |

---

## 3. Arquitectura y Diseño de la Solución

### 3.1 Despacho de Comandos de Memoria y Hardening
En `src/shell/shell.c`, la tabla `BUILTINS[]` fue ampliada para enrutar de manera segura los comandos del Módulo de Memoria:
- `mem-alloc`: valida `<pid>` (entero positivo), `<size_kb>` (> 0), verifica colisión de PID en bloques activos y despacha `memory_alloc()`.
- `mem-free`: valida `<pid>` (entero positivo), busca bloques activos y aplica liberación con coalescencia contigua automática (`first_fit_free`).
- `switch-ram`: valida argumentos admitidos (`ff` o `pag`), verifica el estado actual y solicita confirmación interactiva `[s/N]` si hay memoria ocupada.
- `mem-view`: renderiza el mapa dinámico de memoria (`[P10: 64KB][FREE: 64KB]`).
- `mem-page-table` y `mem-translate`: bloqueados en modo First-Fit con mensajes didácticos explicativos y validados con rigor ante PIDs y direcciones en modo Paginación.

### 3.2 Robustez en Productor-Consumidor (`thread-demo`)
En `builtin_thread_demo`:
- Se analizan los parámetros con `strtol`, detectando entradas no numéricas (`thread-demo dos dos diez`) y emitiendo el mensaje correspondiente.
- Se asegura que tanto productores como consumidores sean `>= 2`. Si se pasa `0` o `1` (ej. `thread-demo 0 2 10`), se emite el error `[ERROR] Se requieren minimo 2 productores y 2 consumidores.`.
- Se valida que los ítems por productor sean `> 0`.

### 3.3 Coalescencia y Gestión de Memoria (`first_fit.c` y `memory.c`)
- Se implementó `first_fit_free(pid)` garantizando que la liberación de un bloque fusione automáticamente bloques contiguos marcados como `BLOCK_FREE`, eliminando la fragmentación externa simulada innecesaria.
- `first_fit_view()` genera el esquema visual en terminal sin riesgos de desreferenciación de punteros nulos.

---

## 4. Casos de Prueba Ejecutados

1. **Argumentos vacíos y faltantes:**
   - `mem-alloc` → Muestra sintaxis y ejemplo sin crash.
   - `mem-free` → Muestra sintaxis sin crash.
   - `switch-ram` → Informa que se requiere argumento `[ff|pag]`.
2. **Valores negativos y cero:**
   - `mem-alloc -5 200` → Rechazado: PID no positivo.
   - `mem-alloc 42 0` → Rechazado: tamaño debe ser > 0.
   - `thread-demo 0 2 10` → Rechazado: mínimo 2 productores y consumidores.
   - `thread-demo 2 2 0` → Rechazado: ítems deben ser > 0.
3. **Valores de configuración inválidos:**
   - `mem-config page 50` → Rechazado: no es potencia de 2.
   - `switch-ram invalid` → Rechazado con mensaje de uso.
4. **Duplicación de asignación:**
   - Asignar a PID 10 y reintentar → Muestra advertencia indicando liberar primero.
5. **Liberación inexistente:**
   - `mem-free 999` → Informa que PID 999 no tiene bloques asignados.
