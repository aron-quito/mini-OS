# 📂 Especificaciones Técnicas de Issues (docs/issues/)

Este directorio contiene las especificaciones técnicas detalladas y desacopladas para cada una de las tareas (*Issues*) planificadas en el sistema de gestión del proyecto (Linear / `PROJECT_MANAGEMENT.md`).

---

## 📋 Índice de Issues de Módulo 0: Shell & Infraestructura Base

| Identificador Linear | Issue | Título | Estado | Documento de Especificación |
|:---:|:---:|:---|:---:|:---:|
| **SIS-38** | 1.1 | Inicializar repositorio Git y estructura de carpetas | ✅ Completado (`Done`) | [SIS-38: Inicialización y Estructura](sis-38.md) |
| **SIS-39** | 1.2 | Implementar el ciclo principal del shell (REPL) | ⏳ En Progreso / Listo (`Todo`) | [SIS-39: Ciclo Principal REPL](sis-39.md) |
| **SIS-40** | 1.3 | Parser y tokenizador de línea de comandos | ✅ Completado (`Done`) | *Mergeado en PR #2* |
| **SIS-41** | 1.4 | Dispatcher central y comandos built-in básicos | ✅ Completado (`Done`) | *Mergeado en PR #2* |

---

## 📋 Índice de Issues de Módulo 1: Procesos

| Identificador Linear | Issue | Título | Estado | Documento de Especificación |
|:---:|:---:|:---|:---:|:---:|
| **SIS-42** | 2.1 | Ejecución foreground con fork, execvp y waitpid | ✅ Completado (`Done`) | *Mergeado en PR #3* |
| **SIS-43** | 2.2 | Ejecución en background (&) y reserva de memoria | 🔍 En Revisión (`In Review`) | [SIS-43: Background y Memoria](sis-43.md) |
| **SIS-44** | 2.3 | Tabla de control de procesos en background | ✅ Completado (`Done`) | *Implementado en `main`* |
| **SIS-45** | 2.4 | Comando ps-mini — Visualizador de procesos | ✅ Completado (`Done`) | [SIS-45: Comando ps-mini](sis-45.md) |

---

## 📋 Índice de Issues de Módulo 3: Gestión de Memoria (First-Fit & Paginación)

| Identificador Linear | Issue | Título | Estado | Documento de Especificación |
|:---:|:---:|:---|:---:|:---:|
| **SIS-46** | 3.1 | Estructuras de datos de memoria y lista de bloques | ✅ Completado (`Done`) | [SIS-46: Estructuras de Memoria y Lista de Bloques](sis-46.md) |
| **SIS-47** | 3.2 | Comandos de configuración de RAM y presets (`mem-config`, `mem-status`) | ✅ Completado (`Done`) | [SIS-47: Configuración de RAM y Presets](sis-47.md) |
| **SIS-48** | 3.3 | Algoritmo First-Fit — Asignación contigua (`ff_alloc`) y split | ✅ Completado (`Done`) | [SIS-48: Asignación Contigua First-Fit y Split](sis-48.md) |
| **SIS-49** | 3.4 | Algoritmo First-Fit — Liberación (`ff_free`) y coalescencia | ✅ Completado (`Done`) | [SIS-49: Liberación First-Fit y Coalescencia](sis-49.md) |
| **SIS-50** | 4.1 | Estructura de cola FIFO para procesos en espera (`mem_queue`) | ✅ Completado (`Done`) | *Mergeado en PR #14* |
| **SIS-53** | 4.4 | Desencolado reactivo y asignación automática (`mem_queue_process_waiting`) | ✅ Completado (`Done`) | [SIS-53: Desencolado Reactivo FIFO](sis-53.md) |
| **SIS-54** | 5.1 | Estructuras de paginación y asignación de marcos (`pag_alloc`) | ✅ Completado (`Done`) | [SIS-54: Estructuras de Paginación y Asignación de Marcos](sis-54.md) |
| **SIS-55** | 5.2 | Traducción de dirección lógica a física (`mem-translate`) | ✅ Completado (`Done`) | [SIS-55: Traducción de Dirección Lógica a Física](sis-55.md) |
| **SIS-56** | 5.3 | Visualizador de tabla de páginas (`mem-page-table`) y `pag_free` | ✅ Completado (`Done`) | [SIS-56: Tabla de Páginas y Liberación de Marcos](sis-56.md) |
| **SIS-57** | 6.1 | Visualizador gráfico interactivo de RAM (`mem-view` / `ram-view`) | ✅ Completado (`Done`) | [SIS-57: Visualizador Gráfico de RAM](sis-57.md) |
| **SIS-58** | 6.2 | Comando `switch-ram` con capa de seguridad interactiva | ✅ Completado (`Done`) | [SIS-58: Comando switch-ram Interactivo](sis-58.md) |
| **SIS-59** | 6.3 | Transición atómica y reset seguro de estructuras en `switch-ram` | ✅ Completado (`Done`) | [SIS-59: Transición Atómica en switch-ram](sis-59.md) |

---

## 📋 Índice de Issues de Hitos 8 y 9: Estabilización, Auditoría y Documentación Final

| Identificador Linear | Issue | Título | Estado | Documento de Especificación |
|:---:|:---:|:---|:---:|:---:|
| **SIS-64..67** | 8.1–8.4 | Señales POSIX (`sigaction`), validaciones de entrada, pruebas integrales y Valgrind/ASan | ✅ Completado (`Done`) | [SIS-64..67: Estabilización y Auditoría](sis-64-67.md) |
| **SIS-68..69** | 9.1–9.2 | `README.md` integral e Informe Técnico Formal (`docs/INFORME_TECNICO.md`) | ✅ Completado (`Done`) | [SIS-68..69: Documentación e Informe Técnico](sis-68-69.md) |

---

## 📌 Guía de Uso
- Cada documento `sis-XX.md` delimita el **alcance estricto** de la tarea correspondiente para evitar la implementación prematura o acoplamiento no deseado entre fases.
- Los archivos en `src/` que pertenezcan a hitos posteriores deben mantenerse como **stubs mínimos** hasta que su issue correspondiente entre en fase de desarrollo.
