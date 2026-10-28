# MiniOS — Shell Interactivo con Gestión de Procesos, Hilos y Memoria

> **Proyecto Universitario — Sistemas Operativos**  
> Aplicación de consola en C11 para entornos Linux con arquitectura modular.

---

## 📚 Documentación del Proyecto

- 📋 **[Plan de Gestión del Proyecto (Milestones e Issues)](docs/PROJECT_MANAGEMENT.md)**: Planificación completa en 9 Milestones y 33 Issues con código C de referencia, dependencias y ruta crítica.
- 📂 **[Especificación de Casos de Uso (docs/use-cases/)](docs/use-cases/UC-INDEX.md)**: Casos de uso detallados para cada interacción del sistema (UC-00 a UC-06).

---

## 🧩 Módulos del Sistema y Features Destacadas

| Módulo | Descripción | Syscalls / Primitivas Clave |
|:---|:---|:---|
| **Shell Base** | REPL interactivo con prompt `miniOS> `, parser de comandos y dispatcher. | `fgets`, `strtok_r`, `write` |
| **Módulo 1 — Procesos** | Ejecución en foreground y background (`&`), cosecha de procesos y visualizador `ps-mini`. | `fork`, `execvp`, `waitpid`, `sigaction` |
| **Módulo 3 — Memoria** | Simulación de **First-Fit** y **Paginación** con traducción de direcciones (`mem-translate`).<br>• **Cola de espera de RAM (`mem_queue`)**: Encolado automático ante RAM insuficiente o fragmentación y auto-asignación reactiva al liberar.<br>• **Visualizador de RAM (`mem-view`)**: Gráfico interactivo en terminal de bloques/marcos y estado de cola.<br>• **Switch Seguro (`switch-ram`)**: Conmutación en caliente con confirmación interactiva `[s/N]` si hay procesos activos en RAM o cola.<br>• **Configuración de memoria**: Tamaños de RAM, marcos y presets (`mem-config default` / `ff default marcos`). | Simulación matemática en memoria dinámica |
| **Módulo 2 — Hilos** | Modelo Productor-Consumidor con búfer circular compartido (mínimo 2 productores y 2 consumidores) garantizando **0 pérdidas de datos**. | `pthread_create`, `pthread_join`, `pthread_mutex_t`, `pthread_cond_t` |

---

## 🚩 Resumen de Milestones

1. **🚩 M1: Infraestructura Base — Shell CLI y Repositorio** (Shell / CLI Base)
2. **🚩 M2: Módulo 1 — Gestión de Procesos y Concurrencia** (Módulo 1: Procesos)
3. **🚩 M3: Módulo 3 — Memoria: Configuración Global, Presets y First-Fit** (Módulo 3: Memoria)
4. **🚩 M4: Módulo 3 — Memoria: Cola de Espera por RAM Insuficiente (`mem_queue`)** (Módulo 3: Memoria)
5. **🚩 M5: Módulo 3 — Memoria: Paginación y Traducción de Direcciones** (Módulo 3: Memoria)
6. **🚩 M6: Módulo 3 — Memoria: Visualizador Terminal (`mem-view`) y Switch Seguro (`switch-ram`)** (Módulo 3: Memoria)
7. **🚩 M7: Módulo 2 — Hilos: Productor-Consumidor con pthread** (Módulo 2: Hilos)
8. **🚩 M8: Integración Global, Manejo de Señales y Validación de Robustez** (Sistema / Integración)
9. **🚩 M9: Entregables Académicos Finales, Documentación y Video Demostrativo** (Entregables Finales)

---

## 🛠️ Compilación y Ejecución

```bash
# Clonar el repositorio
git clone https://github.com/tu-usuario/mini-OS.git
cd mini-OS

# Compilar con Makefile
make

# Ejecutar el shell
./mini-os
```
