# MiniSO - Shell Interactivo con Gestion de Procesos, Hilos y Memoria

> Proyecto universitario — Sistemas Operativos | Aplicacion de consola en C/Linux

## Documentacion

- [Gestion del Proyecto (Milestones e Issues)](docs/PROJECT_MANAGEMENT.md)
- [Casos de Uso](docs/use-cases/UC-INDEX.md)

## Modulos

| Modulo | Descripcion | Syscalls/Primitivas |
|--------|-------------|---------------------|
| Shell Base | REPL con prompt miniOS> | fgets, printf |
| Modulo 1 — Procesos | Fork/exec/wait, concurrencia, ps-mini | fork, execvp, waitpid, getpid |
| Modulo 3 — Memoria | First-Fit, Paginacion, switch-ram | Simulacion matematica |
| Modulo 2 — Hilos | Productor-Consumidor | pthread_create, mutex, cond |

## Compilar y ejecutar

```bash
git clone https://github.com/tu-usuario/mini-OS.git
cd mini-OS
make
./mini-os
```
