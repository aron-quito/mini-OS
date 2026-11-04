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
| **SIS-49** | 3.4 | Algoritmo First-Fit — Liberación (`ff_free`) y coalescencia | 🔒 Bloqueado por SIS-48 | *Próximamente* |

---

## 📌 Guía de Uso
- Cada documento `sis-XX.md` delimita el **alcance estricto** de la tarea correspondiente para evitar la implementación prematura o acoplamiento no deseado entre fases.
- Los archivos en `src/` que pertenezcan a hitos posteriores deben mantenerse como **stubs mínimos** hasta que su issue correspondiente entre en fase de desarrollo.
