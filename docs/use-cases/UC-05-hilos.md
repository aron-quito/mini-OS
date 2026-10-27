# Casos de Uso — UC-05: Módulo 2 — Hilos Productor-Consumidor

**Módulo:** Módulo 2 — Hilos (pthreads)
**Primitivas:** `pthread_create`, `pthread_join`, `pthread_mutex_t`, `pthread_cond_t`
**Comandos:** `thread-demo`, `thread-stats`
**Actor:** Usuario de consola Linux

---

## UC-05-01: Lanzar la demo con parámetros por defecto (2 prod, 2 cons, 10 ítems)

**ID:** UC-05-01
**Nombre:** Ejecutar la demo productor-consumidor con valores predeterminados
**Pre-condición:** El shell está en ejecución.
**Post-condición:** La demo completa; el shell reporta estadísticas. 0 pérdidas de datos.

### Flujo Principal
```
>> thread-demo

<< [HILOS] Iniciando demo: 2 productores, 2 consumidores, 10 ítems c/u
<< [HILOS] Capacidad del búfer: 8 slots
<< [HILOS] Inicio: 2026-10-26 23:30:00
<< ---------------------------------------------------------
<< [PROD 1] Produjo:  42 | Búfer: 1/8
<< [PROD 2] Produjo:  17 | Búfer: 2/8
<< [CONS 1] Consumió: 42 | Búfer: 1/8
<< [PROD 1] Produjo:  88 | Búfer: 2/8
<< [CONS 2] Consumió: 17 | Búfer: 1/8
<< [PROD 2] Produjo:  55 | Búfer: 2/8
<< [CONS 1] Consumió: 88 | Búfer: 1/8
<< ... (continúa hasta que todos los productores terminan)
<< ---------------------------------------------------------
<< [HILOS] Todos los productores terminaron. Señalizando consumidores...
<< [HILOS] Demo finalizada.
<<
<< ╔══════════════════════════════════════════════════════╗
<< ║           ESTADÍSTICAS — Sesión de Hilos             ║
<< ╠══════════════════════════════════════════════════════╣
<< ║  Productores        : 2                              ║
<< ║  Consumidores       : 2                              ║
<< ║  Ítems producidos   : 20                             ║
<< ║  Ítems consumidos   : 20                             ║
<< ║  Pérdida de datos   : 0  [OK - NINGUNA PERDIDA]     ║
<< ║  Capacidad del búfer: 8 slots                        ║
<< ║  Tiempo de ejecución: 2.437 segundos                 ║
<< ╚══════════════════════════════════════════════════════╝
<< miniOS>
```

### Criterios de aceptación críticos
- `Ítems producidos == Ítems consumidos` (20 == 20)
- `Pérdida de datos: 0` — el enunciado lo exige explícitamente
- El shell recupera el prompt DESPUÉS de que todos los hilos terminen (`pthread_join`)

---

## UC-05-02: Lanzar la demo con parámetros personalizados

**ID:** UC-05-02
**Nombre:** Ejecutar thread-demo con número de hilos y/o ítems personalizados
**Pre-condición:** El shell está en ejecución.
**Post-condición:** La demo corre con los parámetros especificados.

### Caso 1: 3 productores, 4 consumidores, 5 ítems c/u
```
>> thread-demo 3 4 5

<< [HILOS] Iniciando demo: 3 productores, 4 consumidores, 5 ítems c/u
<< ... (salida en tiempo real)
<<
<< ║  Productores        : 3                              ║
<< ║  Consumidores       : 4                              ║
<< ║  Ítems producidos   : 15 (3 * 5)                    ║
<< ║  Ítems consumidos   : 15                             ║
<< ║  Pérdida de datos   : 0  [OK]                       ║
```

### Caso 2: Muchos ítems, búfer se satura brevemente
```
>> thread-demo 2 2 50

<< [PROD 1] Produjo:  33 | Búfer: 8/8
<< [PROD 1] ESPERANDO — búfer lleno (pthread_cond_wait not_full)...
<< [CONS 2] Consumió: XX | Búfer: 7/8
<< [PROD 1] Continúa — espacio disponible (pthread_cond_signal not_full)
<< ... (100 ítems totales)
<<
<< ║  Ítems producidos   : 100                           ║
<< ║  Ítems consumidos   : 100                           ║
<< ║  Pérdida de datos   : 0  [OK]                       ║
```

### Criterios de aceptación
- El sistema funciona con cualquier combinación >= 2 prod y >= 2 cons
- Cuando el búfer está lleno, los productores bloquean con `pthread_cond_wait` (no busy-wait)
- Cuando el búfer está vacío, los consumidores bloquean correctamente

---

## UC-05-03: Comportamiento cuando el búfer está lleno (productor bloqueado)

**ID:** UC-05-03
**Nombre:** Verificar que los productores esperan cuando el búfer está lleno
**Pre-condición:** El búfer tiene capacidad = 8. Los consumidores son muy lentos.
**Post-condición:** Los productores se bloquean sin busy-waiting; se desbloquean cuando hay espacio.

### Flujo detallado del mecanismo interno:
```
Estado del búfer: count = 8/8 (LLENO)
Productor intenta insertar:
  pthread_mutex_lock(&buf->mutex);
  while (buf->count == BUFFER_SIZE) {
      // PRODUCTOR BLOQUEADO AQUÍ
      pthread_cond_wait(&buf->not_full, &buf->mutex);
      // [Mutex liberado mientras espera]
  }

Consumidor extrae un elemento:
  buf->count--;
  pthread_cond_signal(&buf->not_full);  // Despierta al productor
  pthread_mutex_unlock(&buf->mutex);

Productor se desbloquea:
  // Continúa tras el while
  buf->data[buf->head] = item;
  buf->count++;
  pthread_cond_signal(&buf->not_empty);
  pthread_mutex_unlock(&buf->mutex);
```

### Observación esperada en la salida:
```
<< [PROD 1] Produjo:  77 | Búfer: 8/8
<< [INFO] Productor 1 bloqueado — búfer lleno, esperando espacio...
<< [CONS 2] Consumió: 99 | Búfer: 7/8
<< [INFO] Productor 1 desbloqueado — espacio disponible.
<< [PROD 1] Produjo:  23 | Búfer: 8/8
```

---

## UC-05-04: Comportamiento cuando el búfer está vacío (consumidor bloqueado)

**ID:** UC-05-04
**Nombre:** Verificar que los consumidores esperan cuando el búfer está vacío
**Pre-condición:** El búfer está vacío. Los productores son lentos.
**Post-condición:** Los consumidores se bloquean sin busy-waiting.

### Flujo del mecanismo interno:
```
Estado del búfer: count = 0 (VACÍO)
Consumidor intenta extraer:
  pthread_mutex_lock(&buf->mutex);
  while (buf->count == 0 && !buf->stop) {
      // CONSUMIDOR BLOQUEADO AQUÍ
      pthread_cond_wait(&buf->not_empty, &buf->mutex);
  }
  if (buf->stop && buf->count == 0) {
      pthread_mutex_unlock(&buf->mutex);
      return -1; // señal de parada
  }

Productor inserta:
  buf->count++;
  pthread_cond_signal(&buf->not_empty); // Despierta al consumidor
```

### Observación esperada en la salida:
```
<< [INFO] Consumidor 2 bloqueado — búfer vacío, esperando datos...
<< [PROD 1] Produjo:  45 | Búfer: 1/8
<< [INFO] Consumidor 2 desbloqueado — datos disponibles.
<< [CONS 2] Consumió: 45 | Búfer: 0/8
```

---

## UC-05-05: Parada ordenada de los consumidores

**ID:** UC-05-05
**Nombre:** Verificar que los consumidores terminan correctamente tras el flag stop
**Pre-condición:** Todos los productores han terminado de producir.
**Post-condición:** Los consumidores vacían el búfer y terminan limpiamente.

### Flujo:
```
[Todos los productores terminaron: pthread_join(&prod[i]) completado]

pthread_mutex_lock(&buf.mutex);
buf.stop = 1;
pthread_cond_broadcast(&buf.not_empty); // Despertar TODOS los consumidores
pthread_mutex_unlock(&buf.mutex);

Consumidores:
  - Si buf.count > 0: consumir el elemento restante
  - Si buf.count == 0 && buf.stop == 1: retornar NULL (terminar)

[pthread_join(&cons[i]) espera a cada consumidor]
```

### Salida esperada al final:
```
<< [CONS 1] Consumió: 12 | Búfer: 0/8
<< [CONS 2] Consumió: 07 | Búfer: 0/8
<< [INFO] Consumidor 1: señal de parada recibida. Terminando...
<< [INFO] Consumidor 2: señal de parada recibida. Terminando...
<< [HILOS] Todos los hilos finalizados.
```

---

## UC-05-06: thread-stats después de la demo

**ID:** UC-05-06
**Nombre:** Consultar estadísticas de la última sesión de hilos
**Pre-condición:** Se ha ejecutado al menos una vez `thread-demo`.
**Post-condición:** Se muestran estadísticas de la sesión.

### Flujo Principal
```
>> thread-demo 2 3 8

[... demo ejecutada ...]

>> thread-stats

<< ╔══════════════════════════════════════════════════════╗
<< ║           ESTADÍSTICAS — Sesión de Hilos             ║
<< ╠══════════════════════════════════════════════════════╣
<< ║  Productores        : 2                              ║
<< ║  Consumidores       : 3                              ║
<< ║  Ítems producidos   : 16  (2 prod * 8 ítems)        ║
<< ║  Ítems consumidos   : 16                             ║
<< ║  Pérdida de datos   : 0  [OK - SIN PERDIDAS]        ║
<< ║  Capacidad del búfer: 8 slots                        ║
<< ║  Tiempo de ejecución: 1.832 segundos                 ║
<< ╚══════════════════════════════════════════════════════╝
<< miniOS>
```

### Flujo Alternativo: sin sesión previa
```
>> thread-stats
<< [INFO] No hay sesión de hilos previa.
<< Use 'thread-demo' para iniciar una demostración.
<< miniOS>
```

---

## UC-05-07: Argumentos inválidos en thread-demo

**ID:** UC-05-07
**Nombre:** Verificar manejo de errores con argumentos inválidos
**Pre-condición:** El shell está en ejecución.
**Post-condición:** Se muestra error; no se lanzan hilos.

### Caso 1: Menos de 2 productores
```
>> thread-demo 1 2 10
<< [ERROR] El número de productores debe ser >= 2 (se recibió: 1).
<< Uso: thread-demo [num_prod] [num_cons] [items]
<< Defaults: thread-demo 2 2 10
<< miniOS>
```

### Caso 2: Menos de 2 consumidores
```
>> thread-demo 2 1 10
<< [ERROR] El número de consumidores debe ser >= 2 (se recibió: 1).
<< miniOS>
```

### Caso 3: Ítems negativos o cero
```
>> thread-demo 2 2 0
<< [ERROR] El número de ítems por productor debe ser > 0.
<< miniOS>
```

### Caso 4: Argumento no numérico
```
>> thread-demo dos dos diez
<< [ERROR] Argumentos inválidos. Deben ser enteros positivos.
<< Uso: thread-demo [num_prod] [num_cons] [items_por_prod]
<< miniOS>
```

---

## UC-05-08: Verificar ausencia de condiciones de carrera

**ID:** UC-05-08
**Nombre:** Garantía de exclusión mutua — no hay race conditions
**Pre-condición:** La demo corre con 2+ productores y 2+ consumidores simultáneos.
**Post-condición:** El contador de ítems producidos siempre iguala al de consumidos.

### Invariante del sistema:
```
INVARIANTE: total_producido == total_consumido (al finalizar todos los hilos)

Mecanismo de garantía:
  - Todo acceso a buf->count protegido por pthread_mutex_lock
  - No hay operaciones count++ / count-- fuera del mutex
  - Los contadores globales produced_count y consumed_count usan operaciones atómicas
    (__sync_fetch_and_add o pthread_mutex protegido)
```

### Herramienta de verificación (opcional, para el informe):
```bash
# Ejecutar con ThreadSanitizer para detectar data races
gcc -fsanitize=thread -g -o mini-os-tsan src/*.c src/**/*.c -lpthread
./mini-os-tsan
# Luego ejecutar: thread-demo 4 4 100
# Esperado: 0 data race warnings
```
