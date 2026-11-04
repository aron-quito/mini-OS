# SIS-45: Comando `ps-mini` — Visualizador de procesos de la sesión (Issue 2.4)

> **Módulo:** Módulo 1 — Procesos  
> **Depende de:** SIS-44 (Issue 2.3 — Tabla de control `bg_table`)  
> **Estado:** ✅ Completado  
> **Rama:** `feature/issue-2.4-ps-mini`

---

## 1. Descripción

Implementar el comando interno `ps-mini` que visualiza en formato tabular estructurado
la lista de todos los procesos en background lanzados durante la sesión actual del shell,
incluyendo su PID, PPID, comando, memoria RAM reservada y estado actual.

---

## 2. Diseño e Implementación

### 2.1 Arquitectura de capas

```
shell_dispatch()           → detecta "ps-mini" como built-in
    └─ builtin_ps_mini()   → delega en la capa de proceso
        └─ process_list_mini()  → itera s_bg_table[] y formatea la tabla
```

### 2.2 Función principal: `process_list_mini()`

**Archivo:** `src/process/process.c`  
**Declaración:** `src/process/process.h`

```c
void process_list_mini(void);
```

**Flujo interno:**

1. Recorre `s_bg_table[0..MAX_BG_PROCESSES-1]` buscando slots con `used == 1`.
2. Si la tabla está vacía, imprime un mensaje informativo y retorna.
3. Imprime cabecera Unicode.
4. Por cada slot ocupado, formatea una fila con:
   - **#**: contador 1-indexed
   - **PID**: `p->pid` (cast a `int`)
   - **PPID**: `p->ppid` (cast a `int`)
   - **Comando**: `p->command` truncado a 20 chars via `snprintf`
   - **RAM (KB)**: `p->mem_kb` si `PROC_RUNNING`, `"0 KB"` si ya terminó
   - **Estado**: resultado del switch sobre `p->status`
5. Imprime pie de tabla Unicode.

### 2.3 Estados del proceso

| Valor enum     | Texto en tabla  | Condición                         |
|----------------|-----------------|-----------------------------------|
| `PROC_RUNNING` | `RUNNING`       | Proceso hijo sigue activo         |
| `PROC_FINISHED`| `DONE (N)`      | Terminó normalmente; N = exit code|
| `PROC_KILLED`  | `KILLED (S)`    | Terminado por señal S             |

### 2.4 Formato de salida

```
╔════╦════════╦════════╦══════════════════════╦══════════════╦══════════════╗
║ #  ║ PID    ║ PPID   ║ Comando              ║ RAM (KB)     ║ Estado       ║
╠════╬════════╬════════╬══════════════════════╬══════════════╬══════════════╣
║ 1  ║ 24891  ║ 24800  ║ sleep 10             ║ 256 KB       ║ RUNNING      ║
║ 2  ║ 24895  ║ 24800  ║ ls -la               ║ 0 KB         ║ DONE (0)     ║
╚════╩════════╩════════╩══════════════════════╩══════════════╩══════════════╝
```

Anchos de columna fijos (alineación left `%-Nf`):

| Columna   | Ancho |
|-----------|-------|
| #         | 2     |
| PID       | 6     |
| PPID      | 6     |
| Comando   | 20    |
| RAM (KB)  | 12    |
| Estado    | 12    |

### 2.5 Registro del built-in en shell

**Archivo:** `src/shell/shell.c`

```c
/* Tabla BUILTINS[] */
{ "ps-mini", builtin_ps_mini },   /* Issue 2.3/2.4 (SIS-44/45) */

/* Handler */
static int builtin_ps_mini(Command *cmd) {
    (void)cmd;
    process_list_mini();
    return 0;
}
```

---

## 3. Ayuda integrada (`help ps-mini`)

```
  ps-mini
    Muestra la tabla de procesos en background de la sesion actual (Issue 2.4).

    Columnas:
      #          Numero de orden en la tabla (1-indexed).
      PID        ID del proceso hijo lanzado.
      PPID       ID del proceso padre (el shell, siempre el mismo).
      Comando    Cadena del comando ejecutado (truncada a 20 caracteres).
      RAM (KB)   Kilobytes reservados en la RAM simulada.
                 Muestra 0 KB si el proceso ya termino y la memoria fue liberada.
      Estado     RUNNING  = proceso activo en este momento.
                 DONE(N)  = proceso terminado con codigo de salida N.
                 KILLED(S)= proceso terminado por la senal S.

    Formato de salida:
      Tabla Unicode con bordes dobles (UTF-8). Hasta 32 entradas.

    Ejemplo:
      miniOS> sleep 10 256 &
      miniOS> ls &
      miniOS> ps-mini

      Nota: solo registra procesos lanzados con '&'. Los procesos en
      foreground no aparecen en la tabla.
```

---

## 4. Criterios de aceptación

| Criterio | Estado | Verificación |
|---|---|---|
| Muestra columnas: #, PID, PPID, Comando, RAM (KB), Estado | ✅ | `echo "sleep 5 & \n ps-mini" \| ./mini-os` |
| Diferencia RUNNING vs DONE/KILLED | ✅ | Lanzar proceso, esperar que termine, volver a correr `ps-mini` |

---

## 5. Comandos de prueba

```bash
# Compilar
make

# Prueba básica: tabla vacía
echo -e "ps-mini\nexit" | ./mini-os

# Prueba completa: 3 procesos, estados mixtos
echo -e "sleep 5 256 &\nsleep 2 128 &\nls &\nps-mini\nsleep 3\nps-mini\nexit" | ./mini-os

# Ayuda del comando
echo -e "help ps-mini\nexit" | ./mini-os
```

**Salida esperada (segunda llamada a ps-mini):**
```
╔════╦════════╦════════╦══════════════════════╦══════════════╦══════════════╗
║ #  ║ PID    ║ PPID   ║ Comando              ║ RAM (KB)     ║ Estado       ║
╠════╬════════╬════════╬══════════════════════╬══════════════╬══════════════╣
║ 1  ║ XXXXX  ║ XXXXX  ║ sleep 5              ║ 256 KB       ║ RUNNING      ║
║ 2  ║ XXXXX  ║ XXXXX  ║ sleep 2              ║ 0 KB         ║ DONE (0)     ║
║ 3  ║ XXXXX  ║ XXXXX  ║ ls                   ║ 0 KB         ║ DONE (0)     ║
╚════╩════════╩════════╩══════════════════════╩══════════════╩══════════════╝
```

---

## 6. Limitaciones conocidas

| Limitación | Descripción | Solución futura |
|---|---|---|
| Solo procesos background | Los comandos en foreground no se registran | Extender `process_execute_fg()` para insertar en la tabla con estado `PROC_FINISHED` al retornar |
| Tabla fija en tamaño | 32 slots estáticos; slots terminados no se liberan durante la sesión | Reemplazar array estático por lista enlazada dinámica |
| Comando truncado | `command[128]` limita cadenas largas; `snprintf` trunca a 20 chars en la tabla | Aumentar buffer o agregar columna expandible |

---

*Especificación técnica generada el 2026-11-03 | Issue 2.4 (SIS-45) | Autor: Antigravity AI*
