# 📂 Especificaciones Técnicas de Issues (docs/issues/)

Este directorio contiene las especificaciones técnicas detalladas y desacopladas para cada una de las tareas (*Issues*) planificadas en el sistema de gestión del proyecto (Linear / `PROJECT_MANAGEMENT.md`).

---

## 📋 Índice de Issues de Módulo 0: Shell & Infraestructura Base

| Identificador Linear | Issue | Título | Estado | Documento de Especificación |
|:---:|:---:|:---|:---:|:---:|
| **SIS-38** | 1.1 | Inicializar repositorio Git y estructura de carpetas | ✅ Completado (`Done`) | [SIS-38: Inicialización y Estructura](sis-38.md) |
| **SIS-39** | 1.2 | Implementar el ciclo principal del shell (REPL) | ⏳ En Progreso / Listo (`Todo`) | [SIS-39: Ciclo Principal REPL](sis-39.md) |
| **SIS-40** | 1.3 | Parser y tokenizador de línea de comandos | 🔒 Bloqueado por SIS-39 | *Próximamente* |
| **SIS-41** | 1.4 | Dispatcher central y comandos built-in básicos | 🔒 Bloqueado por SIS-40 | *Próximamente* |

---

## 📋 Índice de Issues de Módulo 3: Gestión de Memoria (First-Fit & Paginación)

| Identificador Linear | Issue | Título | Estado | Documento de Especificación |
|:---:|:---:|:---|:---:|:---:|
| **SIS-46** | 3.1 | Estructuras de datos de memoria y lista de bloques | ✅ Completado (`Done`) | [SIS-46: Estructuras de Memoria y Lista de Bloques](sis-46.md) |
| **SIS-47** | 3.2 | Comandos de configuración de RAM y presets (`mem-config`) | 🔒 Bloqueado por SIS-46 | *Próximamente* |
| **SIS-48** | 3.3 | Algoritmo First-Fit — Asignación contigua (`ff_alloc`) y split | 🔒 Bloqueado por SIS-46 | *Próximamente* |
| **SIS-49** | 3.4 | Algoritmo First-Fit — Liberación (`ff_free`) y coalescencia | 🔒 Bloqueado por SIS-48 | *Próximamente* |

---

## 📌 Guía de Uso
- Cada documento `sis-XX.md` delimita el **alcance estricto** de la tarea correspondiente para evitar la implementación prematura o acoplamiento no deseado entre fases.
- Los archivos en `src/` que pertenezcan a hitos posteriores deben mantenerse como **stubs mínimos** hasta que su issue correspondiente entre en fase de desarrollo.
