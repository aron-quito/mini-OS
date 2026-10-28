# 📂 Casos de Uso — MiniOS

Este directorio contiene la especificación detallada de todos los casos de uso
del sistema MiniOS. Cada archivo describe una interacción posible con el sistema.

## Índice de Casos de Uso

| Archivo | Módulo | Descripción |
|---------|--------|-------------|
| [UC-00-shell-base.md](UC-00-shell-base.md) | Shell Base | Interacciones básicas del shell (REPL, help, clear, exit) |
| [UC-01-procesos.md](UC-01-procesos.md) | Módulo 1: Procesos | Gestión de procesos con fork/exec/wait, background y ps-mini |
| [UC-02-memoria-ff.md](UC-02-memoria-ff.md) | Módulo 3: Memoria | Particionamiento dinámico contiguo (bloque gigante inicial, First-Fit y split/coalescencia) |
| [UC-03-memoria-paginacion.md](UC-03-memoria-paginacion.md) | Módulo 3: Memoria | Algoritmo de Paginación pura (marcos físicos, tablas de páginas y traducción matemática) |
| [UC-04-switch-ram.md](UC-04-switch-ram.md) | Módulo 3: Memoria | Conmutación segura de algoritmos [ff|pag] y reseteo de estructuras de memoria |
| [UC-05-hilos.md](UC-05-hilos.md) | Módulo 2: Hilos | Productor-Consumidor con pthread y verificación de 0 pérdidas |
| [UC-06-cola-memoria.md](UC-06-cola-memoria.md) | Módulo 3: Memoria | Cola de espera (mem_queue) por RAM insuficiente y auto-asignación |

## Convenciones de Formato

- **Actor principal:** Usuario (interacción en terminal Linux)
- **Sistema:** Shell MiniOS (`miniOS> `)
- **Pre-condición:** Estado requerido antes de ejecutar el caso de uso
- **Post-condición:** Estado del sistema tras ejecutar el caso de uso
- **Flujo principal:** Secuencia de pasos del caso de uso feliz
- **Flujos alternativos:** Variaciones y casos de error

## Convención de Salida

```
>> líneas enviadas por el USUARIO
<< líneas de RESPUESTA del sistema
```
