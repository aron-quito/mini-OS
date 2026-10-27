# Casos de Uso — UC-00: Shell Base

**Módulo:** Shell Base (REPL)
**Actor:** Usuario de consola Linux
**Sistema:** MiniSO Shell (`miniOS> `)

---

## UC-00-01: Iniciar el Shell

**ID:** UC-00-01
**Nombre:** Iniciar el Shell MiniSO
**Pre-condición:** El binario `./mini-os` ha sido compilado exitosamente con `make`.
**Post-condición:** El shell está en ejecución y muestra el prompt `miniOS> `.

### Flujo Principal
```
Usuario ejecuta: ./mini-os

<< ╔══════════════════════════════════╗
<< ║   MiniSO Shell v1.0              ║
<< ║   Sistemas Operativos — 2026     ║
<< ╚══════════════════════════════════╝
<< miniOS>
```

### Criterios de validación
- El prompt `miniOS> ` aparece exactamente (con el espacio al final)
- El proceso del shell es visible en `ps` del sistema con su propio PID
- El shell puede recibir comandos inmediatamente

---

## UC-00-02: Mostrar ayuda con `help`

**ID:** UC-00-02
**Nombre:** Listar comandos disponibles
**Pre-condición:** El shell está en ejecución.
**Post-condición:** Se muestra la lista de comandos; el shell retorna al prompt.

### Flujo Principal
```
>> help

<< ╔══════════════════════════════════════════════════════════════════╗
<< ║                    COMANDOS MiniSO                               ║
<< ╠══════════════╦═══════════════════════════════════════════════════╣
<< ║ SHELL BASE   ║                                                   ║
<< ║ help         ║ Muestra esta ayuda                                ║
<< ║ exit         ║ Termina el shell                                  ║
<< ║ clear        ║ Limpia la pantalla                                ║
<< ╠══════════════╬═══════════════════════════════════════════════════╣
<< ║ MÓDULO 1     ║ PROCESOS                                          ║
<< ║ <cmd> [args] ║ Ejecuta un programa del sistema                   ║
<< ║ <cmd> &      ║ Ejecuta en segundo plano (background)             ║
<< ║ ps-mini      ║ Muestra procesos de la sesión                     ║
<< ╠══════════════╬═══════════════════════════════════════════════════╣
<< ║ MÓDULO 3     ║ MEMORIA                                           ║
<< ║ mem-config   ║ Configura parámetros de RAM                       ║
<< ║ mem-status   ║ Estado actual de la memoria                       ║
<< ║ mem-alloc    ║ Asigna memoria a un proceso                       ║
<< ║ mem-free     ║ Libera memoria de un proceso                      ║
<< ║ mem-view     ║ Visualizador gráfico de RAM                       ║
<< ║ mem-page-table ║ Tabla de páginas de un proceso                  ║
<< ║ mem-translate ║ Traduce dirección lógica a física                ║
<< ║ switch-ram   ║ Cambia algoritmo de memoria [ff|pag]              ║
<< ╠══════════════╬═══════════════════════════════════════════════════╣
<< ║ MÓDULO 2     ║ HILOS                                             ║
<< ║ thread-demo  ║ Demo productor-consumidor con pthreads            ║
<< ║ thread-stats ║ Estadísticas de la última sesión de hilos         ║
<< ╚══════════════╩═══════════════════════════════════════════════════╝
<< miniOS>
```

---

## UC-00-03: Salir con `exit`

**ID:** UC-00-03
**Nombre:** Terminar el shell con comando exit
**Pre-condición:** El shell está en ejecución.
**Post-condición:** El proceso del shell termina con código 0.

### Flujo Principal
```
>> exit
<< [MiniSO] Cerrando shell. ¡Hasta luego!
[Retorna al terminal del SO con código 0]
```

### Flujo Alternativo: exit con procesos en background
```
>> sleep 100 &
<< [1] PID: 5678 lanzado en background
>> exit
<< ADVERTENCIA: Hay 1 proceso(s) en background activos.
<< ¿Desea terminarlos y salir? [s/N]: s
<< [Procesos terminados]
<< [MiniSO] Cerrando shell. ¡Hasta luego!
```

---

## UC-00-04: Salir con Ctrl+D (EOF)

**ID:** UC-00-04
**Nombre:** Terminar el shell con EOF
**Pre-condición:** El shell está en ejecución.
**Post-condición:** El proceso del shell termina limpiamente.

### Flujo Principal
```
[El usuario presiona Ctrl+D]
<< [MiniSO] EOF recibido. Cerrando...
[Retorna al terminal del SO]
```

---

## UC-00-05: Limpiar pantalla con `clear`

**ID:** UC-00-05
**Nombre:** Limpiar la pantalla del terminal
**Pre-condición:** El shell está en ejecución con texto en pantalla.
**Post-condición:** La pantalla se limpia y el prompt aparece en la parte superior.

### Flujo Principal
```
>> clear
[Pantalla limpia — equivalente a ANSI escape: \033[2J\033[H]
<< miniOS>
```

---

## UC-00-06: Comando desconocido

**ID:** UC-00-06
**Nombre:** Ingresar un comando no reconocido
**Pre-condición:** El shell está en ejecución.
**Post-condición:** Se muestra error descriptivo; el shell continúa funcionando.

### Flujo Principal
```
>> xyz_comando_inexistente

<< miniOS: 'xyz_comando_inexistente': comando no encontrado
<< Escribe 'help' para ver los comandos disponibles.
<< miniOS>
```

### Flujo Alternativo: programa externo inexistente
```
>> programa_que_no_existe_en_PATH

<< [ERROR execvp] No such file or directory: 'programa_que_no_existe_en_PATH'
<< miniOS>
```

### Criterio de aceptación
- El shell NO se cae (no crash)
- El error se muestra en stderr (idealmente)
- El prompt reaparece inmediatamente

---

## UC-00-07: Línea vacía (Enter sin texto)

**ID:** UC-00-07
**Nombre:** Presionar Enter sin escribir nada
**Pre-condición:** El shell está en ejecución.
**Post-condición:** El shell ignora la entrada y muestra el prompt nuevamente.

### Flujo Principal
```
>> [Enter vacío]
<< miniOS>
>> [Enter vacío]
<< miniOS>
```

### Criterio de aceptación
- No crash, no error, no comportamiento inesperado
- El prompt simplemente reaparece

---

## UC-00-08: Interrupción con Ctrl+C (SIGINT)

**ID:** UC-00-08
**Nombre:** Interrumpir el shell con Ctrl+C
**Pre-condición:** El shell está en ejecución mostrando el prompt.
**Post-condición:** El shell NO termina; muestra un nuevo prompt.

### Flujo Principal
```
>> [El usuario presiona Ctrl+C]

<< miniOS>
```

### Criterio de aceptación
- El manejador `SIGINT` captura la señal
- El shell NO muere (a diferencia de muchos programas de consola)
- Se imprime un salto de línea antes del nuevo prompt
