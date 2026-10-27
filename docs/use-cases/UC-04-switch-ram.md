# Casos de Uso — UC-04: Comando switch-ram y Capa de Seguridad

**Módulo:** Módulo 3 — Memoria (Switch de Algoritmo)
**Comando:** `switch-ram [ff|pag]`
**Actor:** Usuario de consola Linux

---

## UC-04-01: switch-ram con memoria libre (cambio inmediato)

**ID:** UC-04-01
**Nombre:** Cambiar de algoritmo First-Fit a Paginación con memoria libre
**Pre-condición:** Algoritmo actual = First-Fit. No hay procesos con memoria asignada.
**Post-condición:** El algoritmo cambia a Paginación sin intervención del usuario.

### Flujo Principal
```
>> mem-status
<< Algoritmo: First-Fit | Libre: 1024 KB (100%)

>> switch-ram pag

<< [INFO] Memoria libre. Cambiando algoritmo...
<< [OK] Algoritmo cambiado a: Paginación
<< [INFO] Estructuras First-Fit limpiadas. Tabla de marcos reinicializada.
<< miniOS>

>> mem-status
<< Algoritmo: Paginación | Marcos: 16 | Libres: 16 | Usado: 0 KB
```

### Criterios de aceptación
- **NO** se muestra pregunta de confirmación
- Las estructuras del algoritmo anterior se limpian completamente
- `mem-status` refleja el nuevo algoritmo inmediatamente

---

## UC-04-02: switch-ram con memoria ocupada (confirmación positiva)

**ID:** UC-04-02
**Nombre:** Cambiar de algoritmo cuando hay procesos con memoria asignada — el usuario confirma
**Pre-condición:** Algoritmo = First-Fit. Hay 2 procesos con memoria asignada.
**Post-condición:** El algoritmo cambia a Paginación; toda la memoria es liberada.

### Estado previo:
```
>> mem-alloc 10 256
>> mem-alloc 20 128
>> mem-view
<< [P10: 256KB][P20: 128KB][FREE: 640KB]
```

### Flujo Principal
```
>> switch-ram pag

<< ADVERTENCIA: Hay 2 proceso(s) en RAM y 1 en cola de espera.
<<              El cambio a 'pag' liberará toda la memoria y purgará la cola.
<< ¿Desea continuar? [s/N]: s

<< [INFO] Liberando memoria de PID 10... OK
<< [INFO] Liberando memoria de PID 20... OK
<< [INFO] Purgando cola de espera (1 proceso cancelado)... OK
<< [INFO] Estructuras First-Fit limpiadas.
<< [OK] Algoritmo cambiado a: Paginación
<< [INFO] 16 marcos físicos disponibles.
<< miniOS>

>> mem-status
<< Algoritmo: Paginación | Marcos: 16 | Libres: 16 | Usado: 0 KB
```

### Criterios de aceptación
- La pregunta muestra el número exacto de procesos afectados
- La respuesta 's' (minúscula) o 'S' (mayúscula) ambas se aceptan
- La confirmación debe escribirse completa (solo Enter sería 'N' por default)
- Tras el cambio, `mem-page-table` de PID 10 o 20 devuelve error (ya no existen)

---

## UC-04-03: switch-ram con memoria ocupada (confirmación negativa — cancelado)

**ID:** UC-04-03
**Nombre:** Intentar cambiar algoritmo con memoria ocupada — el usuario cancela
**Pre-condición:** Algoritmo = First-Fit. Hay procesos con memoria asignada.
**Post-condición:** El algoritmo NO cambia. La memoria permanece intacta.

### Flujo Principal (respuesta 'N')
```
>> mem-alloc 10 256
>> switch-ram pag

<< ADVERTENCIA: Hay 1 proceso(s) con memoria asignada.
<<              El cambio liberará TODA la memoria. ¿Desea continuar? [s/N]: N

<< [INFO] Operación cancelada. No se realizaron cambios.
<< miniOS>

>> mem-status
<< Algoritmo: First-Fit | Libre: 768 KB | Usado: 256 KB
<< [P10: 256 KB asignados — sin cambios]
```

### Flujo Alternativo (solo presionar Enter = 'N' por default)
```
<< ¿Desea continuar? [s/N]: [Enter]
<< [INFO] Operación cancelada. No se realizaron cambios.
```

### Criterios de aceptación
- El estado de la memoria NO se modifica en absoluto
- El algoritmo permanece en First-Fit
- `mem-view` muestra el mismo estado que antes del intento de switch

---

## UC-04-04: switch-ram con argumento inválido

**ID:** UC-04-04
**Nombre:** Usar switch-ram con un argumento desconocido
**Pre-condición:** El shell está en ejecución.
**Post-condición:** Se muestra error; el algoritmo no cambia.

### Caso 1: argumento incorrecto
```
>> switch-ram best-fit
<< [ERROR] Algoritmo desconocido: 'best-fit'.
<< Uso: switch-ram [ff|pag]
<<   ff  = First-Fit (bloques contiguos)
<<   pag = Paginación (marcos no contiguos)
<< miniOS>
```

### Caso 2: sin argumentos
```
>> switch-ram
<< [ERROR] Se requiere un argumento.
<< Uso: switch-ram [ff|pag]
<< miniOS>
```

### Caso 3: intentar cambiar al mismo algoritmo
```
[Algoritmo actual: First-Fit]
>> switch-ram ff
<< [INFO] El algoritmo ya es First-Fit. No se realizaron cambios.
<< miniOS>
```

---

## UC-04-05: Ciclo completo FF → PAG → FF

**ID:** UC-04-05
**Nombre:** Alternar entre algoritmos múltiples veces en una sesión
**Pre-condición:** El shell está en ejecución con memoria configurada.
**Post-condición:** El sistema es consistente después de múltiples switches.

### Flujo Principal
```
[Inicio: First-Fit]
>> mem-alloc 1 200
>> switch-ram pag
<< ¿Desea continuar? [s/N]: s
<< [OK] Algoritmo cambiado a: Paginación

>> mem-alloc 1 200       # Ahora en modo PAG
>> mem-page-table 1
<< [Tabla de páginas válida para PID 1 en modo PAG]

>> switch-ram ff
<< ¿Desea continuar? [s/N]: s
<< [OK] Algoritmo cambiado a: First-Fit

>> mem-alloc 1 200       # Ahora en modo FF de nuevo
>> mem-view
<< [P1: 200KB (ajustado al bloque)][FREE: ...]
```

### Criterios de aceptación
- Cada switch limpia completamente las estructuras del algoritmo anterior
- No hay datos residuales de algoritmos anteriores causando comportamiento incorrecto
- El sistema funciona correctamente tras múltiples cambios
