# Recomendaciones Técnicas — miniOS

> [!NOTE]
> **Propósito de este documento:**  
> Este archivo recopila **única y exclusivamente mejoras técnicas y optimizaciones arquitectónicas a futuro que NO forman parte del alcance de las issues planificadas** en el proyecto (`PROJECT_MANAGEMENT.md`). Su objetivo es servir como bitácora de técnicas avanzadas de nivel producción (utilizadas en Bash, Zsh o el kernel de Linux) que exceden los requerimientos académicos actuales, para considerarlas en iteraciones posteriores o proyectos derivados.

---

## 1. Verificación de comandos: `access()` vs Pipe de error post-`execvp()`

### Módulo afectado
`src/process/process.c` — funciones `process_execute_fg()` y `process_execute_bg()`

### Contexto del problema
En la Issue 2.1 se detectó que, al escribir un comando inexistente (ej. `ono`), el sistema mostraba un PID antes de reportar el error. Esto ocurría porque `fork()` se llamaba antes de verificar si el comando existía, y era el proceso **hijo** quien descubría el fallo dentro de `execvp()`.

### Solución implementada (educativa)
Se agregó la función auxiliar `resolve_command_path()` que busca el binario en el `$PATH` del sistema usando `access(name, X_OK)` **antes de llamar a `fork()`**. Si no lo encuentra, el error se reporta directamente desde el proceso padre y se retorna `127` sin crear ningún proceso hijo.

```c
// Verificación previa al fork
if (!resolve_command_path(argv[0])) {
    fprintf(stderr, "miniOS: comando no encontrado: '%s'\n", argv[0]);
    return 127;
}
pid_t pid = fork(); // Solo se ejecuta si el binario existe
```

**Ventaja:** Simple, fácil de entender y elimina el PID fantasma.

### Limitación: Condición de carrera TOCTOU

Esta solución tiene una debilidad conocida en Sistemas Operativos llamada **TOCTOU** (*Time-Of-Check vs Time-Of-Use*):

> Entre el momento en que `access()` verifica la existencia del archivo y el momento en que `execvp()` lo ejecuta realmente, existe una **ventana de tiempo** en la que el archivo podría desaparecer, que le cambien los permisos o que sea reemplazado por otro proceso del sistema.

```text
access("ls", X_OK)   ← OK, existe y es ejecutable
                     ← (otro proceso borra /bin/ls en este momento)
fork()
execvp("ls", ...)    ← Falla con ENOENT, aunque access() lo había confirmado
```

Aunque en la práctica esta carrera es extremadamente improbable en un entorno de pruebas, es una **condición insegura por diseño** que los shells de producción evitan.

### Solución profesional: Pipe de error entre padre e hijo

Los shells reales (Bash, Zsh, Dash) utilizan un **pipe anónimo** creado antes del `fork()` para que el proceso hijo comunique al padre si `execvp()` falló, sin hacer ninguna búsqueda manual previa en el PATH:

```text
Padre                            Hijo
  |                                |
  |── pipe[0] (lectura) ─────────► |── pipe[1] (escritura, solo si execvp falla)
  |                                |
  |──── fork() ──────────────────► |
  |                                |── execvp()
  |                                |    ├─ Éxito: el proceso se reemplaza, el fd del
  |                                |    │         pipe se cierra automáticamente (CLOEXEC)
  |                                |    └─ Falla: write(pipe[1], &errno, sizeof(errno))
  |                                |              exit(127)
  |                                |
  |← read(pipe[0]) ───────────────|
  |    ├─ Leyó errno  → execvp falló → mostrar error sin haber mostrado PID
  |    └─ EOF (pipe cerrado) → execvp tuvo éxito → mostrar PID, esperar con waitpid
```

**Pseudocódigo de referencia:**

```c
int errpipe[2];
pipe(errpipe);
fcntl(errpipe[1], F_SETFD, FD_CLOEXEC); // Se cierra automáticamente en execvp exitoso

pid_t pid = fork();
if (pid == 0) {
    close(errpipe[0]);
    execvp(argv[0], argv);
    // Solo llega aquí si execvp falló
    write(errpipe[1], &errno, sizeof(errno));
    exit(127);
} else {
    close(errpipe[1]);
    int child_errno = 0;
    ssize_t n = read(errpipe[0], &child_errno, sizeof(child_errno));
    close(errpipe[0]);

    if (n > 0) {
        // execvp falló: leer el errno enviado por el hijo
        if (child_errno == ENOENT) {
            fprintf(stderr, "miniOS: comando no encontrado: '%s'\n", argv[0]);
        } else {
            fprintf(stderr, "miniOS: error al ejecutar '%s': %s\n", argv[0], strerror(child_errno));
        }
        waitpid(pid, NULL, 0); // Cosechar el proceso hijo que murió con exit(127)
        return 127;
    }
    // execvp tuvo éxito: el pipe se cerró por CLOEXEC, continuar normalmente
    printf("[Proceso lanzado] PID: %d | PPID: %d (miniOS)\n\n", pid, getpid());
    // ... waitpid() bloqueante ...
}
```

**Ventajas de la solución profesional:**
- **Sin TOCTOU:** No hay verificación previa; `execvp()` es quien hace la búsqueda y el pipe entrega el resultado real.
- **Sin duplicación de lógica:** No hay que reimplementar el algoritmo de búsqueda en `$PATH`; `execvp()` ya lo hace internamente.
- **Comunicación IPC real:** Usa pipes anónimos, una primitiva fundamental de UNIX.

### Recomendación
Refactorizar `process_execute_fg()` y `process_execute_bg()` para usar la técnica del pipe de error únicamente si se desea elevar la robustez del shell más allá de las especificaciones de la materia.
