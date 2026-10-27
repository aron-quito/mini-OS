# Casos de Uso — UC-01: Módulo 1 — Gestión de Procesos

**Módulo:** Módulo 1 — Procesos
**Syscalls clave:** `fork()`, `execvp()`, `waitpid()`, `getpid()`, `getppid()`
**Actor:** Usuario de consola Linux

---

## UC-01-01: Ejecutar programa externo (modo foreground)

**ID:** UC-01-01
**Nombre:** Ejecutar un comando del sistema operativo
**Pre-condición:** El shell está en ejecución. El programa existe en el PATH.
**Post-condición:** El programa se ejecuta, termina, y el shell muestra el resumen de estado.

### Flujo Principal
```
>> ls -la /tmp

<< [Proceso lanzado] PID: 4521 | PPID: 4500 (miniOS)
<< total 48
<< drwxrwxrwt 12 root root 4096 Oct 26 23:10 .
<< drwxr-xr-x 19 root root 4096 Oct 20 10:00 ..
<< drwxr-xr-x  2 user user 4096 Oct 26 22:30 tmux-1000
<< ... (salida de ls)
<< [PID 4521] FINALIZADO | Código de salida: 0
<< miniOS>
```

### Detalle técnico
```
fork() crea PID=4521 (hijo)
  hijo: execvp("ls", ["ls", "-la", "/tmp", NULL])
  padre(PID=4500): waitpid(4521, &status, 0) -- bloquea
hijo termina -> padre recibe status -> WIFEXITED(status)=true, WEXITSTATUS(status)=0
```

### Flujo Alternativo: comando falla (e.g., argumento incorrecto)
```
>> ls --argumento-invalido-xyz

<< [Proceso lanzado] PID: 4522 | PPID: 4500 (miniOS)
<< ls: unrecognized option '--argumento-invalido-xyz'
<< [PID 4522] FINALIZADO | Código de salida: 2
<< miniOS>
```

### Criterios de aceptación
- El PID y PPID se muestran correctamente
- El shell NO se cae si el comando retorna código != 0
- El código de salida exacto se reporta

---

## UC-01-02: Ejecutar comando externo inexistente

**ID:** UC-01-02
**Nombre:** Intentar ejecutar un programa que no existe en el PATH
**Pre-condición:** El shell está en ejecución.
**Post-condición:** Se muestra un error descriptivo; el shell continúa.

### Flujo Principal
```
>> programa_fantasma arg1 arg2

<< [Proceso lanzado] PID: 4523 | PPID: 4500 (miniOS)
<< [execvp ERROR] No such file or directory: 'programa_fantasma'
<< [PID 4523] FINALIZADO | Código de salida: 1
<< miniOS>
```

### Detalle técnico
```
fork() crea hijo PID=4523
  hijo: execvp("programa_fantasma", ...) -> retorna -1, errno=ENOENT
  hijo: perror("execvp") -> imprime error
  hijo: exit(EXIT_FAILURE)
padre: waitpid(4523) -> WEXITSTATUS=1
```

### Criterios de aceptación
- El mensaje de error proviene de `perror("execvp")` en el proceso hijo
- El shell padre NO se afecta y continúa funcionando

---

## UC-01-03: Ejecutar proceso en background con `&`

**ID:** UC-01-03
**Nombre:** Lanzar proceso en segundo plano
**Pre-condición:** El shell está en ejecución.
**Post-condición:** El proceso corre en background; el shell muestra el prompt inmediatamente.

### Flujo Principal
```
>> sleep 10 &

<< [1] PID: 4524 lanzado en background
<< miniOS>

[El shell está disponible inmediatamente para nuevos comandos]
```

### Detalle técnico
```
fork() crea hijo PID=4524
  hijo: execvp("sleep", ["sleep", "10", NULL])
  padre: NO llama waitpid() bloqueante
  padre: registra PID=4524 en bg_table[0]
  padre: vuelve al prompt inmediatamente
```

---

## UC-01-04: Ejecución concurrente de 2 procesos en background

**ID:** UC-01-04
**Nombre:** Verificar concurrencia de al menos 2 procesos simultáneos
**Pre-condición:** El shell está en ejecución.
**Post-condición:** Dos procesos corren simultáneamente sin interferirse.

### Flujo Principal
```
>> sleep 8 &
<< [1] PID: 4530 lanzado en background

>> sleep 5 &
<< [2] PID: 4531 lanzado en background

>> ps-mini
<< ╔════╦════════╦════════╦═══════════════╦══════════════╗
<< ║ #  ║ PID    ║ PPID   ║ Comando       ║ Estado       ║
<< ╠════╬════════╬════════╬═══════════════╬══════════════╣
<< ║ 1  ║ 4530   ║ 4500   ║ sleep 8       ║ RUNNING      ║
<< ║ 2  ║ 4531   ║ 4500   ║ sleep 5       ║ RUNNING      ║
<< ╚════╩════════╩════════╩═══════════════╩══════════════╝
<< miniOS>

[Después de ~5 segundos, PID 4531 termina — cosechado por SIGCHLD handler]
[Después de ~8 segundos, PID 4530 termina — cosechado por SIGCHLD handler]

>> ps-mini
<< ╔════╦════════╦════════╦═══════════════╦══════════════╗
<< ║ #  ║ PID    ║ PPID   ║ Comando       ║ Estado       ║
<< ╠════╬════════╬════════╬═══════════════╬══════════════╣
<< ║ 1  ║ 4530   ║ 4500   ║ sleep 8       ║ DONE (0)     ║
<< ║ 2  ║ 4531   ║ 4500   ║ sleep 5       ║ DONE (0)     ║
<< ╚════╩════════╩════════╩═══════════════╩══════════════╝
<< miniOS>
```

### Criterios de aceptación
- Los dos procesos corren simultáneamente (verificable con `ps aux` del sistema)
- El manejador SIGCHLD cosecha los hijos sin crear zombies
- `ps-mini` refleja el estado actualizado tras la terminación

---

## UC-01-05: Proceso terminado por señal

**ID:** UC-01-05
**Nombre:** Detectar proceso hijo terminado por señal (SIGKILL)
**Pre-condición:** Un proceso está corriendo en background.
**Post-condición:** El estado KILLED con el número de señal se reporta en `ps-mini`.

### Flujo Principal
```
>> sleep 100 &
<< [1] PID: 4540 lanzado en background

[Desde otra terminal: kill -9 4540]

>> ps-mini
<< ║ 1  ║ 4540   ║ 4500   ║ sleep 100     ║ KILLED (sig:9 SIGKILL) ║
```

### Detalle técnico
```
SIGCHLD enviado al padre cuando hijo muere
sigchld_handler: waitpid(4540, &status, WNOHANG)
  WIFSIGNALED(status) = true
  WTERMSIG(status) = 9
update_bg_process_status(4540, status) -> bg_table[0].state = KILLED, signal=9
```

---

## UC-01-06: Comando ps-mini sin procesos activos

**ID:** UC-01-06
**Nombre:** Ejecutar ps-mini sin procesos en la tabla
**Pre-condición:** El shell está en ejecución sin haber lanzado procesos en background.
**Post-condición:** Se muestra un mensaje amigable indicando que no hay procesos.

### Flujo Principal
```
>> ps-mini

<< [ps-mini] No hay procesos registrados en esta sesión.
<< miniOS>
```

---

## UC-01-07: Verificación de PID y PPID con getpid()/getppid()

**ID:** UC-01-07
**Nombre:** Verificar que PID y PPID son correctos
**Pre-condición:** El shell está en ejecución.
**Post-condición:** Los PIDs mostrados coinciden con los del sistema operativo.

### Flujo Principal
```
>> echo "hola desde MiniOS"

<< [Proceso lanzado] PID: 4551 | PPID: 4500 (miniOS)
<< hola desde MiniOS
<< [PID 4551] FINALIZADO | Código de salida: 0

[Verificación: en otra terminal durante la ejecución]
>> ps -p 4551 -o pid,ppid,comm
<< PID  PPID COMMAND
<< 4551 4500 echo
```

### Criterios de aceptación
- `PPID` mostrado por MiniOS = PID del proceso shell (verificable con `ps`)
- `PID` mostrado por MiniOS = PID del proceso hijo creado con `fork()`
