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
| **SIS-44** | 2.3 | Tabla de control de procesos en background | ⏳ Siguiente (`Todo`) | *Próximamente* |
| **SIS-45** | 2.4 | Comando ps-mini — Visualizador de procesos | 🔒 Bloqueado por SIS-44 | *Próximamente* |

---

## 📌 Guía de Uso
- Cada documento `sis-XX.md` delimita el **alcance estricto** de la tarea correspondiente para evitar la implementación prematura o acoplamiento no deseado entre fases.
- Los archivos en `src/` que pertenezcan a hitos posteriores deben mantenerse como **stubs mínimos** hasta que su issue correspondiente entre en fase de desarrollo.
