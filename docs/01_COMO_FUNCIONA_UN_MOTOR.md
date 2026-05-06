# Cómo funciona un motor de videojuegos — Estudio técnico en profundidad

> Documento de referencia técnica independiente del proyecto. Cubre los
> principios, algoritmos y decisiones de arquitectura que subyacen a cualquier
> motor de videojuegos moderno, tanto 2D como 3D.

---

## ÍNDICE

1. [El bucle principal](#1-el-bucle-principal)
2. [Gestión de memoria](#2-gestión-de-memoria)
3. [Pipeline de renderizado](#3-pipeline-de-renderizado)
4. [Sistema de entidades](#4-sistema-de-entidades)
5. [Motor de física](#5-motor-de-física)
6. [Sistema de input](#6-sistema-de-input)
7. [Sistema de audio](#7-sistema-de-audio)
8. [Gestión de recursos](#8-gestión-de-recursos)
9. [Sistema de cámara](#9-sistema-de-cámara)
10. [Sistema de animación](#10-sistema-de-animación)
11. [Multithreading en motores](#11-multithreading-en-motores)
12. [Scripting y comunicación entre sistemas](#12-scripting-y-comunicación-entre-sistemas)
13. [Referencia: arquitecturas de motores reales](#13-referencia-arquitecturas-de-motores-reales)

---

## 1. El bucle principal

### 1.1 El problema fundamental del tiempo

Un motor de videojuegos es un programa en tiempo real. A diferencia de una
aplicación de escritorio que espera eventos, el motor debe avanzar la simulación
continuamente. El tiempo es el recurso más crítico de gestionar.

El bucle más ingenuo es:

```cpp
while (running) {
    update();
    render();
}
```

Esto ejecuta tan rápido como el CPU lo permita: 2000 iteraciones por segundo en
un CPU rápido, 800 en uno lento. El juego corre a velocidades distintas en
distinto hardware. Esto era aceptable en los años 80 cuando el hardware era
fijo (NES, SNES), pero es inaceptable hoy.

---

### 1.2 Variable timestep

La solución inmediata es medir cuánto tardó el frame anterior y usar ese tiempo
como unidad de avance:

```cpp
float lastTime = getTime();
while (running) {
    float now = getTime();
    float dt = now - lastTime;
    lastTime = now;

    update(dt);  // "avanza la simulación dt segundos"
    render();
}
```

Un personaje que se mueve a 100 píxeles/segundo calculará:
`position += 100 * dt`

A 60 FPS: `dt ≈ 0.0167s` → mueve 1.67 píxeles por frame.  
A 120 FPS: `dt ≈ 0.0083s` → mueve 0.83 píxeles por frame.  
En ambos casos, 100 píxeles avanzados por segundo real. Correcto.

**Problema: el spike de deltaTime.**  
Si el sistema operativo interrumpe el proceso 200ms (antivirus, actualización,
GC de Java en segundo plano), el siguiente `dt` será 0.2s. El personaje saltaría
20 píxeles de golpe y podría atravesar una pared de 5 píxeles de grosor.

---

### 1.3 Fixed timestep (el estándar de la industria)

La solución definitiva la describió Glenn Fiedler en su artículo "Fix Your
Timestep" (2004) y sigue siendo el estándar hoy:

```cpp
const float FIXED_DT = 1.0f / 120.0f;  // física a 120 Hz
float accumulator = 0.0f;

float lastTime = getTime();
while (running) {
    float now = getTime();
    float frameTime = now - lastTime;
    lastTime = now;

    // Limitar el frameTime para evitar la "espiral de la muerte"
    if (frameTime > 0.05f) frameTime = 0.05f;  // cap en 50ms (20 FPS mínimo)

    accumulator += frameTime;

    // Avanzar la física en pasos fijos
    while (accumulator >= FIXED_DT) {
        physicsUpdate(FIXED_DT);    // determinista, siempre el mismo dt
        accumulator -= FIXED_DT;
    }

    // El alpha representa qué fracción del siguiente paso ya transcurrió
    float alpha = accumulator / FIXED_DT;

    // Interpolar posiciones para renderizado suave
    render(alpha);
}
```

**La espiral de la muerte:** si `physicsUpdate` tarda más que `FIXED_DT`, el
acumulador crece más rápido de lo que se vacía. El cap de 50ms (o similar)
rompe ese ciclo a costa de ralentizar la simulación en frames muy pesados.

**Por qué separar física de render:**  
- La física necesita pasos fijos para ser determinista (replays, multijugador).
- El render puede correr a cualquier frecuencia (60, 144, 240 FPS).
- Con interpolación (`lerp(prevPos, currentPos, alpha)`), el render es suave
  incluso si la física corre más lento que el display.

**Timesteps típicos en la industria:**
| Juego/Motor | Física | Render |
|---|---|---|
| Unity (default) | 50 Hz (0.02s) | Variable o vsync |
| Godot | 60 Hz (configurable) | Variable |
| Unreal Engine | 60 Hz | Variable |
| Box2D recomendado | 60–120 Hz | Variable |

---

### 1.4 Frame rate limiting y VSync

**VSync (Vertical Sync):** el renderer espera el barrido vertical del monitor
antes de presentar el frame. Elimina el "tearing" (ver dos frames parciales
a la vez) a costa de latencia añadida. SDL3: `SDL_SetRenderVSync(renderer, 1)`.

**Frame rate cap sin VSync:**
```cpp
const float TARGET_FPS = 60.0f;
const float MIN_FRAME_TIME = 1.0f / TARGET_FPS;

float frameStart = getTime();
// ... update + render ...
float frameEnd = getTime();
float elapsed = frameEnd - frameStart;
if (elapsed < MIN_FRAME_TIME) {
    sleep(MIN_FRAME_TIME - elapsed);  // SDL_Delay en SDL3
}
```

Esto reduce el consumo de CPU/GPU en juegos que no necesitan más de N FPS,
importante para portátiles y dispositivos móviles.

---

### 1.5 Orden de operaciones dentro del frame

El orden importa. La secuencia estándar:

```
1. Procesar eventos del SO (ventana, input crudo)
2. Actualizar estado de input (JustPressed, JustReleased)
3. Time::Update() — calcular deltaTime
4. FixedUpdate loop — física (N veces si el acumulador lo permite)
5. Update(dt) — lógica de juego, IA, animaciones
6. LateUpdate(dt) — cosas que deben ocurrir DESPUÉS de Update
             (ej: cámara que sigue al personaje ya movido)
7. Renderer::BeginFrame()
8. Render() — recolectar draw calls, ordenar por Z
9. UI Render() — sobre el mundo, ignorando cámara de mundo
10. Renderer::EndFrame() — swap buffers, present
11. Input::EndFrame() — copiar estado actual a estado previo
```

**Por qué LateUpdate:** si la cámara sigue al jugador y ambos se actualizan en
`Update`, puede haber un frame de lag visual dependiendo del orden de ejecución.
`LateUpdate` garantiza que la cámara se posicione DESPUÉS de que todas las
entidades se hayan movido.

---

## 2. Gestión de memoria

### 2.1 Por qué la memoria importa más en juegos que en otras aplicaciones

Los juegos deben entregar un frame cada 16.67ms (60 FPS). Las asignaciones de
heap (`new`, `malloc`) son lentas y no deterministas: el allocator del sistema
puede tardar microsegundos variables. Las fragmentaciones acumuladas durante
horas de juego pueden causar picos de latencia.

Además, la **localidad de caché** (cache locality) tiene un impacto enorme. Un
acceso a memoria en caché L1 tarda ~4 ciclos; un cache miss que va a RAM tarda
~200 ciclos. Si 1000 entidades están dispersas en heap, actualizarlas una por una
genera 1000 cache misses. Si están en un array contiguo, se actualizan con
cache caliente.

---

### 2.2 Pool allocator

Pre-asigna un bloque grande y lo divide en slots fijos. Ideal para objetos del
mismo tipo (entidades, partículas, componentes):

```
[ slot 0 | slot 1 | slot 2 | slot 3 | ... | slot N ]
     ↑         ↑
  ocupado    libre (enlazado en lista libre)
```

- `alloc()`: toma el primer slot de la lista libre. O(1).
- `free(ptr)`: devuelve el slot a la lista libre. O(1).
- No hay fragmentación: todos los slots son del mismo tamaño.
- Los objetos están contiguos en memoria → cache friendly.

```cpp
template<typename T, size_t N>
class Pool {
    alignas(T) std::byte storage[sizeof(T) * N];
    T* freeList[N];
    int top = 0;
public:
    Pool() {
        for (int i = 0; i < N; ++i)
            freeList[i] = reinterpret_cast<T*>(&storage[sizeof(T)*i]);
        top = N;
    }
    T* alloc() { return top > 0 ? freeList[--top] : nullptr; }
    void free(T* p) { freeList[top++] = p; }
};
```

---

### 2.3 Arena / Linear allocator (por frame)

Para datos temporales que solo viven un frame (listas de render, strings de debug,
eventos temporales), un allocator lineal es óptimo:

```
[  datos  |  datos  |  datos  |           libre          ]
 ↑                             ↑
inicio                       cursor

alloc(size): cursor += size; return cursor - size;  // O(1), sin overhead
reset():     cursor = inicio;                        // liberar todo de golpe
```

Al final de cada frame se hace `reset()` y se descarta todo de golpe. No hay
fragmentación, no hay overhead de `free()` individual.

---

### 2.4 SoA vs AoS (Structure of Arrays vs Array of Structures)

**AoS (Array of Structures) — el patrón OOP habitual:**
```cpp
struct Entity {
    float x, y;       // posición
    float vx, vy;     // velocidad
    Sprite sprite;    // render data (grande)
    Collider box;     // física (mediano)
};
Entity entities[1000];
```

Para actualizar posiciones: `entities[i].x += entities[i].vx * dt`  
El CPU carga 64+ bytes por entidad aunque solo necesita 16 bytes (x, y, vx, vy).
El resto (sprite, collider) es ruido que desperdicia líneas de caché.

**SoA (Structure of Arrays) — el patrón ECS/SIMD:**
```cpp
struct PositionSystem {
    float x[1000];
    float y[1000];
    float vx[1000];
    float vy[1000];
};
```

Para actualizar posiciones: solo se cargan arrays de floats contiguos.
El CPU puede vectorizar automáticamente (SSE/AVX: procesar 4 u 8 floats/ciclo).
El sistema de render puede ignorar estos arrays completamente.

Este es el principio fundamental detrás del ECS orientado a datos.

---

## 3. Pipeline de renderizado

### 3.1 El pipeline de la GPU (hardware)

Todo lo que se dibuja pasa por estas etapas en la GPU:

```
Vértices → [Vertex Shader] → Ensamblado de primitivas → Rasterización
         → [Fragment/Pixel Shader] → Tests (depth, stencil) → Framebuffer
```

**Vertex Shader:** transforma posiciones de "espacio local" a "espacio de clip"
aplicando las matrices Model * View * Projection.

**Rasterización:** convierte los triángulos matemáticos en fragmentos (píxeles
candidatos). Interpola los atributos del vértice (UV, color, normal) a cada píxel.

**Fragment Shader:** determina el color final de cada píxel. Aquí viven los
efectos: texturas, iluminación, shadows, etc.

**En SDL2/SDL3:** todo esto está oculto. SDL usa el pipeline de la GPU
internamente pero no lo expone. En OpenGL y Vulkan tienes control total.

---

### 3.2 Transformaciones: el sistema de matrices

En 3D (y 2D homogéneo) toda transformación es una multiplicación de matrices 4×4:

```
Posición final en pantalla = Projection * View * Model * localPosition
```

- **Model matrix:** transforma del espacio local al espacio del mundo
  (posición + rotación + escala de la entidad).
- **View matrix:** representa la cámara. Transforma del espacio del mundo al
  espacio de la cámara. Es la inversa de la transformación de la cámara.
- **Projection matrix:** transforma del espacio de cámara al espacio de clip
  (aplica perspectiva en 3D, o proyección ortográfica en 2D).

En 2D sin perspectiva, la Projection es ortográfica:
```
| 2/W   0    0  -1 |
|  0   2/H   0  -1 |
|  0    0    1   0 |
|  0    0    0   1 |
```
Donde W y H son las dimensiones de la ventana. Transforma píxeles a NDC [-1, 1].

---

### 3.3 Render 2D: Z-order y capas

En 2D no hay depth buffer real (o no se usa), pero el orden de dibujado define
qué aparece encima. Los motores 2D usan varias estrategias:

**Sort por Z explícito:** cada sprite tiene un valor Z. Antes de renderizar se
ordena la lista de sprites por Z ascendente (primero fondo, luego personajes,
luego primer plano). Coste: O(N log N) por frame.

**Capas de render:** grupos predefinidos (`Background`, `Terrain`, `Characters`,
`FX`, `UI`). Dentro de cada capa puede haber un Z relativo. Más predecible que
un Z global.

**Painter's algorithm:** dibujar de atrás hacia adelante. Simple pero no maneja
ciclos (objeto A delante de B, B delante de A al mismo tiempo).

---

### 3.4 Sprite Batching en profundidad

Cada `DrawCall` (llamada a `glDrawArrays` o `SDL_RenderTexture`) tiene un
overhead fijo en la CPU (~0.1ms) y en el bus CPU↔GPU. Con 500 sprites
independientes: 500 drawcalls = posible bottleneck.

**Batching manual (el estándar en 2D):**

1. Acumular geometría en un buffer de vértices en CPU:
   ```
   [ quad0_v0, quad0_v1, quad0_v2, quad0_v3,  quad1_v0, ... ]
   ```
2. Si el siguiente sprite usa la MISMA textura/shader: añadirlo al buffer.
3. Si cambia textura: hacer flush (enviar el buffer acumulado a GPU en UN drawcall),
   luego empezar nuevo buffer.

**Texture Atlas:** agrupar todas las imágenes en una sola textura grande hace que
el batch nunca se rompa por cambio de textura. El motor solo hace 1-2 drawcalls
para todo el mundo.

**Instanced rendering (GPU instancing):** en lugar de enviar geometría repetida,
se envía la geometría UNA vez y una lista de transformaciones. La GPU replica
internamente. Ideal para partículas o tiles idénticos.

---

### 3.5 Forward vs Deferred rendering

Solo relevante para 3D con iluminación, pero importante entenderlo para el futuro:

**Forward rendering:** por cada fragmento, calcular el aporte de TODAS las luces.
`O(fragmentos × luces)`. Simple. Para pocos objetos y pocas luces.

**Deferred rendering:** separar geometría de iluminación en dos pasadas.
1. Geometry Pass: renderizar posición, normal, albedo a un G-Buffer (varias texturas).
2. Lighting Pass: por cada luz, leer el G-Buffer y calcular solo los fragmentos
   afectados. `O(fragmentos de luz × 1)`.

Godot 4 y Unreal usan Deferred por defecto. Unity ofrece ambos.

---

### 3.6 Render Graph (Frame Graph)

Un concepto moderno (introducido por Frostbite en 2017, adoptado por todos los
motores grandes) para gestionar la complejidad de pasos de render:

El render graph es un grafo dirigido acíclico (DAG) donde:
- Los **nodos** son pasos de render (Shadow Pass, GBuffer Pass, Lighting Pass, TAA, Bloom, UI...).
- Las **aristas** son texturas/buffers que fluyen de un paso al siguiente.

El sistema:
1. Recibe la declaración de todos los pasos y sus dependencias.
2. Calcula automáticamente qué recursos necesita crear/destruir.
3. Reordena y paraleliza pasos que no tienen dependencia entre sí.
4. Sincroniza la GPU solo donde es necesario (crítico en Vulkan/DX12).

Para un motor 2D simple esto es innecesario. Para 3D con post-process es esencial.

---

### 3.7 La diferencia entre SDL, OpenGL y Vulkan

Entender estos niveles de abstracción es crucial para diseñar bien el backend:

**SDL3 Renderer:**
- API de alto nivel. "Dibuja este sprite en esta posición."
- SDL decide internamente cómo usar la GPU (puede usar Metal, D3D12, Vulkan, OpenGL).
- Sin acceso a shaders propios (en el modo renderer estándar).
- Ideal para prototipado y 2D simple.

**OpenGL 4.x (Core Profile):**
- API de medio nivel. Control de shaders, buffers, texturas.
- Estado global ("estado de máquina"): cada llamada modifica un estado global
  en el driver, que persiste hasta que lo cambies. Difícil de paralelizar.
- El driver hace mucha optimización invisible (puede reordenar comandos, compilar
  shaders en background). Conveniente pero poco predecible.

**Vulkan:**
- API de bajo nivel. Control total de la GPU.
- Sin estado global. Todos los comandos van a Command Buffers explícitos.
- La sincronización es responsabilidad tuya (semáforos, fences, barriers).
- Los render passes y framebuffers se declaran explícitamente.
- Mucho más verboso (crear un triángulo toma ~800 líneas), pero predecible y eficiente.
- Permite CPU/GPU overlap: mientras la GPU ejecuta el frame N, la CPU prepara el frame N+1.

**Regla práctica:** OpenGL es 10× más código que SDL. Vulkan es 10× más código
que OpenGL. El control adicional solo vale la pena para juegos AAA o cuando
el renderer SDL/GL es el cuello de botella.

---

## 4. Sistema de entidades

### 4.1 El problema del OOP puro en juegos

La herencia profunda de OOP es un antipatrón conocido en game dev:

```
GameObject
  └─ Actor
       ├─ Player
       │    └─ PlayableCharacter
       │         └─ HeroWithSword  ← 5 niveles de herencia
       └─ Enemy
            ├─ FlyingEnemy
            └─ GroundEnemy
                 └─ ArmordGroundEnemy  ← ¿qué métodos overrides cual?
```

Problemas:
- El "diamond problem": si `FlyingArmordEnemy` hereda de `FlyingEnemy` y
  `ArmordGroundEnemy`, ¿cuál `update()` llama?
- Jerarquías rígidas: añadir una nueva capacidad (ej: "puede nadar") requiere
  tocar múltiples ramas.
- Código espagueti de `if (dynamic_cast<SomeSubtype*>(obj))`.

---

### 4.2 Scene Graph

La solución más usada en motores 2D y juegos pequeños/medianos.

**Estructura:**
```
Scene
 ├─ Entity "World"
 │    ├─ Entity "Background" [SpriteComponent]
 │    └─ Entity "Player"     [SpriteComponent, ColliderComponent, ScriptComponent]
 │         └─ Entity "Sword" [SpriteComponent]  ← hijo, hereda transform
 └─ Entity "UI"
      └─ Entity "HealthBar"  [UIComponent]
```

**Transformaciones jerárquicas:**
Cada entidad tiene una transformación local. La transformación mundial se calcula
multiplicando hacia arriba en el árbol:

```cpp
glm::mat3 Entity::GetWorldTransform() const {
    if (parent == nullptr) return localTransform;
    return parent->GetWorldTransform() * localTransform;
}
```

**Dirty flag pattern:** recalcular la transformación mundial en cada frame es
caro si el árbol es grande. Se usa un flag `isDirty`. Cuando un nodo se mueve,
se marca a sí mismo y a todos sus descendientes como dirty. La transformación
solo se recalcula cuando se pide Y el nodo está dirty.

```cpp
void Entity::SetPosition(float x, float y) {
    localTransform.position = {x, y};
    markDirty();  // marca este nodo y todos sus hijos
}

glm::mat3 Entity::GetWorldTransform() {
    if (isDirty) {
        cachedWorldTransform = parent
            ? parent->GetWorldTransform() * localTransform.ToMatrix()
            : localTransform.ToMatrix();
        isDirty = false;
    }
    return cachedWorldTransform;
}
```

---

### 4.3 ECS (Entity-Component System) en profundidad

La arquitectura ECS separa identidad (Entity), datos (Component) y lógica (System):

- **Entity:** solo un entero. `using EntityID = uint32_t;`
- **Component:** struct de datos puros, sin métodos de lógica.
- **System:** clase que itera sobre entidades que tienen ciertos componentes.

```cpp
// Componentes: datos planos
struct Position { float x, y; };
struct Velocity { float vx, vy; };
struct Sprite   { TextureHandle tex; Rect srcRect; };

// Sistema: lógica pura
class MovementSystem {
    void Update(float dt, ComponentStorage& storage) {
        // Iterar solo entidades que tienen AMBOS componentes
        for (auto [pos, vel] : storage.view<Position, Velocity>()) {
            pos.x += vel.vx * dt;
            pos.y += vel.vy * dt;
        }
    }
};
```

**Archetype-based ECS (el modelo más eficiente, usado por Unity DOTS y Bevy):**

Las entidades con los mismos componentes se agrupan en un "archetype". Cada
archetype almacena sus componentes en arrays contiguos separados:

```
Archetype [Position, Velocity, Sprite]:
  positions: [p0, p1, p2, p3, ...]  ← array contiguo
  velocities:[v0, v1, v2, v3, ...]  ← array contiguo
  sprites:   [s0, s1, s2, s3, ...]  ← array contiguo

Archetype [Position, Sprite]:        ← sin velocidad
  positions: [p0, p1, ...]
  sprites:   [s0, s1, ...]
```

Un sistema que necesita `Position + Velocity` itera los archivos que los contengan.
El acceso es perfectamente secuencial en memoria → máximo rendimiento de caché.

**¿Cuándo usar ECS vs Scene Graph?**

| Criterio | Scene Graph | ECS |
|---|---|---|
| < 500 entidades | ✅ Suficiente | ✅ Suficiente |
| 10.000+ entidades | ⚠️ Puede ser lento | ✅ Diseñado para esto |
| Jerarquía padre-hijo | ✅ Natural | ⚠️ Complicado |
| Lerning curve | Baja | Alta |
| Herramientas de debug | Fácil de visualizar | Más difícil |
| Uso en industria 2D | Godot, Cocos2d, Unity 2D | Unity DOTS, Bevy, EnTT |

---

### 4.4 Spatial Partitioning (particionamiento espacial)

Con muchas entidades, preguntar "¿qué entidades están cerca de X?" es O(N) si
se comprueba contra todas. Los sistemas espaciales reducen esto:

**Quadtree (para 2D):**
Divide el espacio recursivamente en 4 cuadrantes. Cada nodo almacena entidades
dentro de su área. Consultar una región: bajar por los nodos que se solapan,
ignorar ramas enteras que no intersectan.

```
┌───────┬───────┐
│  NW   │  NE   │  Si buscas entidades en la esquina NE,
│   *   │  *  * │  solo visitas el nodo NE y sus hijos.
├───────┼───────┤  Los nodos NW, SW, SE no se tocan.
│  SW   │  SE   │
│       │   *   │
└───────┴───────┘
```

**Spatial Hash Grid:** divide el mundo en celdas de tamaño fijo. Cada entidad
se registra en la celda correspondiente. Consultar vecinos: obtener las 9 celdas
alrededor del punto. O(1) promedio. Más simple que quadtree, ideal para entidades
de tamaño similar.

**AABB Tree / BVH (Bounding Volume Hierarchy):** estructura para física y raycast.
Árboles binarios de cajas envolventes. Usado por Box2D, Bullet, Unreal.

---

## 5. Motor de física

### 5.1 Métodos de integración numérica

La física simula el movimiento aplicando fuerzas que se acumulan en aceleración,
velocidad y posición con el tiempo. Esto es una ODE (Ordinary Differential
Equation). Los motores la resuelven numéricamente:

**Euler explícito (el más simple, el peor):**
```cpp
velocity += acceleration * dt;
position += velocity * dt;
```
Error de primer orden: la posición diverge de la solución real con el tiempo.
A dt grande se vuelve inestable (la velocidad crece sin límite). NO recomendado
para física real.

**Symplectic Euler (Semi-Implicit Euler):**
```cpp
velocity += acceleration * dt;   // igual que Euler
position += velocity * dt;       // usa la velocidad YA actualizada
```
Pequeño cambio, gran diferencia: este método es **simplécticamente correcto**,
conserva la energía en sistemas sin fricción. Es lo que usan Box2D y la mayoría
de motores de juego. Simple y estable.

**Verlet (posición):**
```cpp
newPosition = 2*position - prevPosition + acceleration * dt*dt;
prevPosition = position;
position = newPosition;
```
No almacena velocidad explícitamente. La velocidad está implícita en la diferencia
de posiciones. Muy estable. Usado en cloth simulation y partículas.

**Runge-Kutta 4 (RK4):**
Evalúa la función 4 veces por paso. Error de cuarto orden. Mucho más preciso que
Euler pero 4× más caro. Necesario solo cuando la precisión importa más que
la velocidad (simulaciones científicas, no juegos).

**Veredicto para juegos:** Symplectic Euler con timestep fijo es el estándar.
Es lo suficientemente estable y preciso para la mayoría de juegos, y muy barato.

---

### 5.2 Detección de colisiones: tres fases

**Broad Phase (descartar rápidamente):**

El objetivo es reducir los pares de objetos a comprobar de O(N²) a O(N log N)
o menos. Solo objetos que podrían estar cerca se pasan a la siguiente fase.

- **AABB overlapping sweep and prune (SAP):** ordena los objetos por su
  coordenada X (o Y). Solo los objetos solapados en esa coordenada pasan al
  siguiente test. Muy eficiente si los objetos están ordenados entre frames.

- **Spatial Hash/Grid:** los objetos en la misma celda o celdas adyacentes se
  comprueban entre sí.

- **Dynamic AABB Tree (Bounding Volume Hierarchy):** árbol binario de AABBs.
  Box2D usa este método. Inserción O(log N), query O(log N + resultados).

**Midphase (formas simplificadas):**  
Comprueba las formas de colisión simplificadas (AABB, OBB, esferas) de los
candidatos del broad phase. Elimina pares evidentemente no colisionantes sin
llegar al test exacto.

**Narrow Phase (test exacto):**

- **AABB vs AABB:**
  ```
  overlap = (ax1 < bx2 && ax2 > bx1) && (ay1 < by2 && ay2 > by1)
  ```
  O(1). La más barata. Suficiente para muchos juegos 2D.

- **Círculo vs Círculo:**
  ```
  dist² = (cx1-cx2)² + (cy1-cy2)²
  overlap = dist² < (r1+r2)²
  ```
  Sin raíz cuadrada = muy barato.

- **SAT (Separating Axis Theorem):** para polígonos convexos arbitrarios.
  Si existe un eje donde las proyecciones de ambas formas no se solapan, no hay
  colisión. Se prueba cada arista normal de ambos polígonos.

- **GJK (Gilbert-Johnson-Keerthi):** para formas convexas arbitrarias en 3D.
  Trabaja con la "diferencia de Minkowski". Más general que SAT pero más complejo.
  Usado por Bullet Physics.

---

### 5.3 Resolución de colisiones

Detectar que dos objetos se tocan no basta: hay que reaccionar correctamente.

**MTV (Minimum Translation Vector):** el vector más corto que separa los dos
objetos. Mover cada objeto la mitad del MTV en direcciones opuestas.

```cpp
glm::vec2 mtv = calculateMTV(a, b);
if (a.isStatic && !b.isStatic)      b.position += mtv;
else if (!a.isStatic && b.isStatic) a.position -= mtv;
else {
    a.position -= mtv * 0.5f;
    b.position += mtv * 0.5f;
}
```

**Resolución por impulso (impulse-based resolution):**
Más realista que solo separar los objetos. Calcula el impulso necesario para
cambiar las velocidades de forma que la velocidad relativa en el punto de contacto
sea cero (o negativa en caso de rebote):

```
j = -(1 + restitution) * relativeVelocity · normal
    ────────────────────────────────────────────────
           1/massA + 1/massB
```
Donde `restitution` es el coeficiente de rebote (0 = inelástico, 1 = elástico).

**Continuous Collision Detection (CCD) — el problema del tunneling:**
Si un objeto se mueve muy rápido, puede atravesar una pared fina en un solo frame.
La solución es hacer un "sweep" del área barrida por el objeto durante el frame
y detectar la primera colisión.

En 2D: un rectángulo en movimiento barre un área rectangular más grande
(Minkowski sum de la forma + el desplazamiento).

---

### 5.4 Constraints (restricciones)

Más allá de colisiones, los motores de física permiten unir objetos con joints:

- **Distance joint:** dos objetos siempre a distancia fija (cadena, cuerda rígida).
- **Revolute joint:** bisagra. Un objeto gira alrededor de un punto del otro.
- **Prismatic joint:** un objeto solo puede moverse en un eje relativo al otro.
- **Weld joint:** une dos cuerpos rígidamente como si fueran uno.

Box2D implementa todos estos. La resolución usa un **Sequential Impulse Solver**:
aplica impulsos iterativamente (N iteraciones por frame) hasta que todas las
restricciones se satisfacen aproximadamente.

---

## 6. Sistema de input

### 6.1 Polling vs eventos

**Event-based (SDL, Windows WM):** el OS notifica cuándo cambia el estado de
un dispositivo. El motor debe procesar estos eventos en el game loop. Más
reactivo, no pierde eventos entre frames.

**Polling (XInput, raw input):** el motor pregunta directamente al dispositivo
su estado actual. Más simple pero puede perder pulsaciones muy cortas (un
botón pulsado y soltado entre dos polls).

La práctica estándar es usar eventos para teclado/ratón y polling para gamepads
(o combinar ambos).

---

### 6.2 Estado de frame actual vs frame anterior

La diferencia entre "está pulsado" y "se acaba de pulsar":

```cpp
struct InputState {
    bool keys[KEY_COUNT];
    bool mouseButtons[5];
    int mouseX, mouseY;
    int mouseScrollX, mouseScrollY;
};

InputState current, previous;

void EndFrame() {
    previous = current;  // copiar estado al final del frame
}

bool IsKeyHeld(Key k)         { return current.keys[k]; }
bool IsKeyJustPressed(Key k)  { return current.keys[k] && !previous.keys[k]; }
bool IsKeyJustReleased(Key k) { return !current.keys[k] && previous.keys[k]; }
```

---

### 6.3 Input Actions (el sistema maduro)

Los juegos no deberían tener `if (IsKeyPressed(SCANCODE_SPACE)) jump()`.
Deberían tener `if (actions.IsPressed("Jump")) jump()`.

Por qué:
- Permite remapeo de controles sin tocar código de juego.
- La misma acción puede tener múltiples bindings (Space Y botón A del gamepad).
- Los presets de controles (WASD vs ZQSD para teclados AZERTY) son triviales.

```cpp
// Configuración (puede venir de un archivo JSON)
InputMap map;
map.bind("Jump", Key::Space);
map.bind("Jump", GamepadButton::A);
map.bind("MoveLeft", Key::A, AxisSign::Negative);
map.bind("MoveLeft", GamepadAxis::LeftStickX, AxisSign::Negative);

// Código de juego — no sabe qué tecla física es
if (input.IsActionJustPressed("Jump")) player.Jump();
float moveX = input.GetActionAxis("MoveLeft", "MoveRight");
```

---

### 6.4 Gamepad: dead zones y axis normalization

Los joysticks analógicos tienen un centro físico imperfecto. Un joystick en
reposo puede reportar valores como (0.05, -0.03) en lugar de (0, 0). La **dead
zone** filtra estos valores para que el centro real sea exactamente 0:

```cpp
float ApplyDeadzone(float value, float deadzone) {
    if (std::abs(value) < deadzone) return 0.0f;
    // Reescalar para que el rango útil empiece en 0, no en deadzone
    float sign = value > 0 ? 1.0f : -1.0f;
    return sign * (std::abs(value) - deadzone) / (1.0f - deadzone);
}
```

**Dead zone radial vs axial:**
- Axial: aplicar la dead zone por separado a X e Y. Produce un cuadrado central
  muerto; movimientos en diagonal tienen el radio muerto distorsionado.
- Radial: calcular la magnitud del vector, aplicar la dead zone a la magnitud,
  normalizar. Más natural para movimiento en todas direcciones.

---

### 6.5 Input buffering (coyote time y buffer de salto)

Técnicas de "game feel" que hacen el input más responsivo:

**Coyote time:** dar al jugador un pequeño margen de tiempo después de salir
de un borde para poder saltar. Si el jugador presiona salto hasta 100ms después
de salir de una plataforma, el salto se ejecuta igualmente:

```cpp
if (IsKeyJustPressed(Key::Space)) {
    if (isGrounded || timeSinceGrounded < COYOTE_TIME)
        Jump();
    else
        jumpBuffered = true;  // guardar el intento
}
if (isGrounded && jumpBuffered && timeSinceJumpBuffer < JUMP_BUFFER_TIME)
    Jump();
```

**Jump buffer:** si el jugador presiona salto 80ms antes de aterrizar, el salto
se ejecuta tan pronto como el personaje toca el suelo. Elimina la frustración de
presionar salto "demasiado pronto".

---

## 7. Sistema de audio

### 7.1 Fundamentos del audio digital

El audio digital es una secuencia de muestras (samples) que representan la
amplitud de la onda sonora en instantes de tiempo discretos.

- **Sample rate:** muestras por segundo. 44100 Hz (CD quality), 48000 Hz (estándar
  de video/juegos). Una muestra es una medida de amplitud en ese instante.
- **Bit depth:** bits por muestra. 16-bit (65536 niveles), 24-bit, 32-bit float.
  Más bits = menos ruido de cuantización.
- **Canales:** mono (1), stereo (2), 5.1 (6).
- **PCM (Pulse Code Modulation):** el formato crudo. Sin compresión.
- **Formatos comprimidos:** MP3, OGG Vorbis, FLAC. El motor los decodifica a PCM
  en memoria antes de reproducirlos.

---

### 7.2 El audio callback

El sistema operativo no permite que el juego "empuje" audio cuando quiera. En
cambio, el SO llama a una función del motor periódicamente (cada ~10ms) pidiendo
el siguiente buffer de samples:

```cpp
void audioCallback(void* userdata, uint8_t* stream, int len) {
    // Mezclar todas las fuentes activas en 'stream'
    // Esta función se llama en un thread separado
    // DEBE ser rápida y no bloquear
    AudioSystem* audio = static_cast<AudioSystem*>(userdata);
    audio->Mix(stream, len);
}
```

Esta función corre en un **thread de audio** separado del game loop. Esto implica:
- No puedes acceder a datos del juego desde aquí sin sincronización.
- El audio buffer tiene una latencia inherente: lo que se escribe aquí se
  escuchará ~10-20ms después.
- La función debe completarse ANTES del siguiente callback o habrá underrun (glitch).

---

### 7.3 Audio mixing

El mixer combina múltiples fuentes de audio en una sola salida:

```cpp
void Mix(int16_t* output, int numSamples) {
    memset(output, 0, numSamples * sizeof(int16_t));
    for (auto& source : activeSources) {
        if (!source.isPlaying) continue;
        for (int i = 0; i < numSamples; ++i) {
            // Sumar la muestra de esta fuente a la salida
            int32_t mixed = output[i] + (int32_t)(source.data[source.cursor] * source.volume);
            // Clamp para evitar overflow (clipping)
            output[i] = (int16_t)std::clamp(mixed, -32768, 32767);
            source.cursor++;
        }
    }
}
```

**Volume, pitch, loop:** el volumen es un multiplicador del sample. El pitch
es una alteración del sample rate (leer muestras más rápido = tono más agudo).
Para loop, se reinicia el cursor al final del buffer.

---

### 7.4 Audio posicional 2D

Para simular sonido posicional en 2D (escuchar de dónde viene algo):

**Atenuación por distancia:**
```cpp
float distanceSq = (listenerPos - sourcePos).LengthSq();
float volume = 1.0f / (1.0f + distanceSq / (maxRange * maxRange));
```

**Panning estéreo:**
```cpp
float angle = atan2(sourcePos.y - listenerPos.y, sourcePos.x - listenerPos.x);
float pan = cos(angle - listenerDirection);  // -1 = izquierda, +1 = derecha
leftVolume  = volume * (1.0f - pan) * 0.5f;
rightVolume = volume * (1.0f + pan) * 0.5f;
```

---

## 8. Gestión de recursos

### 8.1 El ciclo de vida de un activo

```
Archivo en disco → Carga (I/O) → Decodificación (CPU) → Upload a GPU → En uso
                                                                         ↓
                               Liberación de GPU ← Descarga ← Ya no referenciado
```

Cada paso tiene un coste. Los motores intentan hacer la carga **asíncrona**
(en un thread secundario) para no bloquear el game loop.

---

### 8.2 Handle system vs shared_ptr

**Con `shared_ptr`:** 
```cpp
auto texture = resourceManager.Load<Texture>("player.png");
// shared_ptr mantiene el recuento de referencias
// cuando el último shared_ptr se destruye, la textura se descarga
```
Simple pero tiene overhead de atomic reference counting y puede causar
descarga accidental si se guarda en el stack (scope pequeño).

**Con handles opacos:**
```cpp
TextureHandle hTex = resourceManager.Load("player.png");
// hTex es un entero. La vida de la textura la gestiona el ResourceManager.
resourceManager.Release(hTex);  // explícito
```
Más control. El motor puede mover activos en memoria sin invalidar handles.
Permite sistemas de hot-reload más robustos.

---

### 8.3 Carga asíncrona y streaming

Para niveles grandes o juegos open world, no es viable tener todo en memoria.
El streaming carga activos en background mientras el juego corre:

```cpp
// No bloqueante: devuelve un "future" o handle pendiente
AsyncHandle<Texture> future = resourceManager.LoadAsync("bigTexture.png");

// Más tarde en el game loop:
if (future.IsReady()) {
    TextureHandle tex = future.Get();
    // usar tex
}
```

Los activos se cargan en un **I/O thread** dedicado. Cuando terminan, se postea
el resultado a una cola que el thread principal consume al inicio de cada frame.

---

### 8.4 Virtual File System (VFS)

Abstrae el acceso a archivos del SO. Permite:

- Transparentemente leer desde un archivo `.pak` (activos empaquetados para
  distribución) o desde el disco durante desarrollo.
- Hot-reload: si un archivo cambia en disco, el VFS notifica al ResourceManager
  para recargar el activo sin reiniciar el juego.
- Montaje de rutas virtuales: `"textures/player.png"` puede apuntar a
  `/home/user/game/assets/textures/player.png` en dev o a `data.pak/textures/player.png`
  en release.

---

## 9. Sistema de cámara

### 9.1 Cámara 2D: conceptos básicos

La cámara 2D define la transformación que se aplica a todo el mundo antes de
renderizar. Sus propiedades fundamentales:

```cpp
struct Camera2D {
    glm::vec2 position;    // punto del mundo que está en el centro de la pantalla
    float rotation;        // rotación de la cámara (raro en 2D pero útil para efectos)
    float zoom;            // 1.0 = normal, 2.0 = doble de zoom (más cerca)
    glm::vec2 viewport;    // tamaño de la ventana en píxeles
};
```

**Transformación mundo → pantalla:**
```cpp
glm::vec2 WorldToScreen(glm::vec2 worldPos) {
    glm::vec2 relative = worldPos - camera.position;
    relative *= camera.zoom;
    // Aplicar rotación si la hay
    glm::vec2 screenPos = viewport * 0.5f + relative;
    return screenPos;
}
```

**Transformación pantalla → mundo (para mouse picking):**
```cpp
glm::vec2 ScreenToWorld(glm::vec2 screenPos) {
    glm::vec2 relative = screenPos - viewport * 0.5f;
    relative /= camera.zoom;
    return relative + camera.position;
}
```

---

### 9.2 Seguimiento de entidad (follow camera)

Una cámara que sigue al jugador sin cortes bruscos:

**Lerp directo (el más simple):**
```cpp
camera.position = glm::mix(camera.position, target.position, smoothSpeed * dt);
```
Rápido al principio, más lento cuando se acerca. Nunca llega exactamente al target.

**Camera bounds (límites del nivel):**
```cpp
camera.position.x = std::clamp(camera.position.x, 
    viewport.x * 0.5f / zoom,               // borde izquierdo
    levelWidth - viewport.x * 0.5f / zoom   // borde derecho
);
```

**Lookahead:** la cámara mira ligeramente en la dirección de movimiento del
jugador, dando más información visual de lo que viene:
```cpp
glm::vec2 lookahead = player.velocity * LOOKAHEAD_FACTOR;
glm::vec2 targetPos = player.position + lookahead;
camera.position = glm::mix(camera.position, targetPos, smoothSpeed * dt);
```

---

### 9.3 Screen shake

Efecto esencial para game feel. Implementación simple con trauma system:

```cpp
float trauma = 0.0f;  // 0 = sin shake, 1 = máximo shake

void AddTrauma(float amount) {
    trauma = std::clamp(trauma + amount, 0.0f, 1.0f);
}

glm::vec2 GetShakeOffset() {
    float shake = trauma * trauma;  // cuadrático = más dramático
    return glm::vec2(
        randomFloat(-1, 1) * MAX_SHAKE_X * shake,
        randomFloat(-1, 1) * MAX_SHAKE_Y * shake
    );
}

void UpdateCamera(float dt) {
    trauma = std::max(0.0f, trauma - DECAY_RATE * dt);
    camera.offset = GetShakeOffset();
}
```

El shake cuadrático (`trauma²`) se siente más natural: sube rápido a trauma alto
y decae suavemente cuando trauma es bajo.

---

## 10. Sistema de animación

### 10.1 Animación por frames (flipbook)

La forma más simple de animación 2D: cambiar el frame del sprite según el tiempo.

```cpp
struct Animation {
    std::vector<Rect> frames;   // cada frame es una región del sprite sheet
    float frameDuration;        // segundos por frame
    bool loop;
};

struct Animator {
    Animation* current;
    int currentFrame;
    float timer;

    void Update(float dt) {
        timer += dt;
        if (timer >= current->frameDuration) {
            timer -= current->frameDuration;
            currentFrame++;
            if (currentFrame >= current->frames.size()) {
                currentFrame = current->loop ? 0 : current->frames.size() - 1;
            }
        }
    }
    Rect GetCurrentFrame() { return current->frames[currentFrame]; }
};
```

---

### 10.2 Animation State Machine (máquina de estados de animación)

En lugar de cambiar animaciones manualmente, se define una máquina de estados
donde las transiciones ocurren automáticamente al cumplirse condiciones:

```
[Idle] ─── moving == true ──→ [Walk]
[Walk] ─── moving == false ─→ [Idle]
[Walk] ─── jump == true ────→ [Jump]
[Jump] ─── grounded == true ─→ [Land] ──(1 vez)──→ [Idle]
```

```cpp
struct AnimationTransition {
    std::string targetState;
    std::function<bool(AnimatorParams&)> condition;
    bool hasExitTime;  // esperar a que la animación termine antes de transicionar
    float exitTime;    // fracción [0,1] de la animación que debe completarse
};

class AnimationStateMachine {
    std::unordered_map<std::string, Animation*> states;
    std::unordered_map<std::string, std::vector<AnimationTransition>> transitions;
    std::string currentState;

    void Update(float dt, AnimatorParams& params) {
        currentAnimator.Update(dt);
        for (auto& transition : transitions[currentState]) {
            if (transition.condition(params)) {
                TransitionTo(transition.targetState);
                break;
            }
        }
    }
};
```

Este es exactamente el sistema que usa Unity (Animator Controller) y Godot
(AnimationTree).

---

### 10.3 Tweening

Para animaciones de UI, cutscenes y efectos visuales que no son sprites:
mover un elemento del punto A al punto B con una curva de suavizado.

```cpp
struct Tween {
    float* target;
    float from, to;
    float duration, elapsed;
    EasingFn easing;

    void Update(float dt) {
        elapsed = std::min(elapsed + dt, duration);
        float t = elapsed / duration;
        *target = from + (to - from) * easing(t);
    }
    bool IsDone() { return elapsed >= duration; }
};
```

Las funciones de easing más usadas:
- `Linear(t) = t`
- `EaseInQuad(t) = t*t`
- `EaseOutQuad(t) = t*(2-t)`
- `EaseInOutCubic(t) = t<0.5 ? 4t³ : 1 - (-2t+2)³/2`
- `Bounce`, `Elastic`, `Back` (overshooting)

---

## 11. Multithreading en motores

### 11.1 El problema del threading en juegos

Los juegos modernos usan múltiples cores, pero de forma controlada. El problema
es que los datos del juego son compartidos y modificados desde todas partes.

El patrón más común en la industria es el **job system** (sistema de trabajos):

```
Main Thread:   [Submit jobs] ──────────────────────── [Collect results]
Worker 1:              [Job A] ──── [Job C]
Worker 2:              [Job B] ──────────────── [Job D]
Worker 3:                      [Job E] ── [Job F]
```

Los jobs son funciones pequeñas sin estado compartido (o con acceso de solo
lectura). El scheduler los distribuye a los cores disponibles.

---

### 11.2 Game Thread + Render Thread

El modelo más común en motores AAA:

**Main/Game Thread:** ejecuta el game loop: input, lógica, física. Al final del
frame, serializa el estado del juego a un **Render Command Buffer**.

**Render Thread:** lee el Render Command Buffer y envía los draw calls a la GPU.
Puede correr con un frame de lag respecto al Game Thread.

```
Frame N:   [Game Thread: update] → [Render Buffer N] → [Render Thread: draw N]
Frame N+1: [Game Thread: update] → [Render Buffer N+1]
                                                    ↓ (paralelo)
                                   [Render Thread: draw N]
```

Esto permite que la GPU esté siempre ocupada aunque el Game Thread tarde en
preparar el siguiente frame.

---

### 11.3 Sincronización sin locks

Los locks (mutex) son lentos y pueden causar deadlocks. Las alternativas:

**Double buffering de estado:** el game thread escribe en el buffer A mientras
el render thread lee del buffer B. Al final del frame se hace swap. Sin locks,
sin contención.

**Lock-free queues (SPSC - Single Producer Single Consumer):** una cola con
un productor y un consumidor puede implementarse sin locks usando operaciones
atómicas. Ideal para comunicación entre Game Thread y Render Thread.

**Immutable commands:** el Render Command Buffer es write-once (solo el game
thread escribe) y read-once (solo el render thread lee). No hay contención.

---

## 12. Scripting y comunicación entre sistemas

### 12.1 El sistema de eventos / mensajes

Los sistemas del motor deben comunicarse sin acoplarse directamente entre sí.
Un sistema de eventos desacoplado permite esto:

```cpp
// Publisher: no sabe quién escucha
EventSystem::Emit("PlayerDied", {playerId, position});

// Subscriber: no sabe quién emite
EventSystem::Subscribe("PlayerDied", [](const Event& e) {
    spawnRespawnPoint(e.Get<int>("playerId"), e.Get<vec2>("position"));
});
```

Esto se llama también **Observer pattern** o **Signal-Slot system** (terminología
de Qt y Godot).

**Tipos de dispatch:**
- **Immediate:** el evento se procesa instantáneamente cuando se emite. Puede
  causar reentrada si el handler emite otro evento.
- **Queued:** los eventos se encolan y se procesan al inicio del siguiente frame.
  Más seguro, añade un frame de latencia.

---

### 12.2 Scripting con Lua

Los motores indie suelen integrar Lua como lenguaje de scripting para que los
diseñadores de juego puedan modificar comportamientos sin recompilar:

```lua
-- Código Lua en el juego
function OnUpdate(dt)
    if Input.IsKeyPressed("Space") then
        player:Jump()
    end
end
```

La integración funciona exponiendo la API del motor a Lua:

```cpp
// Registrar función C++ en Lua
lua_register(L, "GetDeltaTime", [](lua_State* L) -> int {
    lua_pushnumber(L, Time::GetDeltaTime());
    return 1;
});
```

**Alternativas:** Python (pesado), AngelScript (tipado, sintaxis C-like),
Wren (compacto), C# (Unity usa Mono/IL2CPP).

---

### 12.3 Scene serialization (guardar/cargar escenas)

Las escenas deben poder guardarse en disco (editor) y cargarse en runtime:

**JSON/YAML:** legible por humanos, fácil de depurar. Lento de parsear para
escenas grandes.

```json
{
  "name": "Level1",
  "entities": [
    {
      "id": 1,
      "name": "Player",
      "transform": {"x": 100, "y": 200, "rotation": 0, "scaleX": 1, "scaleY": 1},
      "components": [
        {"type": "SpriteComponent", "texture": "player.png"},
        {"type": "ColliderComponent", "shape": "AABB", "w": 32, "h": 48}
      ]
    }
  ]
}
```

**Binary formats (FlatBuffers, protobuf):** más compactos y rápidos. Usados en
producción. No legibles por humanos sin herramientas.

El sistema de componentes debe soportar **reflection** (saber sus propias
propiedades) para que la serialización sea automática, sin escribir código
específico por cada tipo de componente.

---

## 13. Referencia: arquitecturas de motores reales

### Unity (2D/3D, C#)
- Scene Graph con GameObjects y Components (MonoBehaviour).
- Rendering: SRP (Scriptable Render Pipeline). El juego elige URP (ligero) o HDRP (avanzado).
- Física: PhysX (3D), Box2D (2D).
- Sistema de input nuevo: Input System package (action-based).
- Audio: FMOD opcionalmente, sistema propio por defecto.
- Scripting: C# compilado a IL, ejecutado por Mono/CoreCLR o transpilado a C++ (IL2CPP).
- ECS disponible como paquete separado (DOTS).

### Godot 4 (2D/3D, GDScript/C#/C++)
- Scene Graph con Nodes (todo es un Node). Cada escena es un árbol.
- Rendering: Vulkan como backend principal, OpenGL como fallback.
- Física: Godot Physics (propio, 2D y 3D) o Jolt (opcional en 4.x).
- Señales (Signals): sistema de eventos integrado en el lenguaje.
- Scripting: GDScript (Python-like), C#, o extensiones C++ (GDExtension).
- Sin separación engine/game: el motor ES el editor.

### Unreal Engine (3D, C++)
- Actor/Component system (similar a Scene Graph pero con actores como ciudadanos de primera clase).
- Rendering: Deferred rendering, Lumen (GI dinámica), Nanite (geometría virtual).
- Física: PhysX → Chaos (motor propio desde UE5).
- Blueprint: lenguaje visual de scripting compilado a bytecode.
- Scripting: C++ nativo, recarga en caliente con Live Coding.
- Multijugador: sistema de replicación de red integrado en el engine.

### pygame / SDL (2D, Python/C)
- Sin sistema de entidades. El desarrollador construye el suyo.
- Rendering: SDL_Renderer o OpenGL directo.
- Sin física integrada (usar pymunk/Box2D por separado).
- Sin audio avanzado (SDL_mixer).
- Ideal para aprender, prototipado, juegos pequeños.

---

*Última actualización: 2026-05-05*
