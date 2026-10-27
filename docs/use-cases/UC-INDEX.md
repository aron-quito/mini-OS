# 📂 Casos de Uso — MiniSO

Este directorio contiene la especificación detallada de todos los casos de uso
del sistema MiniSO. Cada archivo describe una interacción posible con el sistema.

## Índice de Casos de Uso

| Archivo | Módulo | Descripción |
|---------|--------|-------------|
| [UC-00-shell-base.md](UC-00-shell-base.md) | Shell Base | Interacciones básicas del shell |
| [UC-01-procesos.md](UC-01-procesos.md) | Módulo 1 | Gestión de procesos con fork/exec/wait |
| [UC-02-memoria-ff.md](UC-02-memoria-ff.md) | Módulo 3 | Configuración y algoritmo First-Fit |
| [UC-03-memoria-paginacion.md](UC-03-memoria-paginacion.md) | Módulo 3 | Algoritmo de Paginación |
| [UC-04-switch-ram.md](UC-04-switch-ram.md) | Módulo 3 | Comando switch-ram y capa de seguridad |
| [UC-05-hilos.md](UC-05-hilos.md) | Módulo 2 | Productor-Consumidor con pthread |

## Convenciones de Formato

- **Actor principal:** Usuario (interacción en terminal)
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
