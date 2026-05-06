# EntityEngine — Plan de acción

> Hoja de ruta de desarrollo: qué hay, qué falta, qué sigue y qué considerar.
> Orientado a construir un motor 2D completo y adaptable a Vulkan/OpenGL/3D.

---

## ÍNDICE

1. [¿Qué tenemos?](#1-qué-tenemos)
2. [¿Qué falta?](#2-qué-falta)
3. [¿Qué sigue? — Fases de desarrollo](#3-qué-sigue--fases-de-desarrollo)
4. [¿Qué hay que tener en cuenta?](#4-qué-hay-que-tener-en-cuenta)

---

## 1. ¿Qué tenemos?

Una base sólida y limpia de bajo nivel. Los cimientos del motor están bien
estructurados aunque incompletos:

| Sistema | Estado | ¿Usable? |
|---|---|---|
| Bucle principal (`Application`) | ✅ Funcional | Sí, pero sin punto de extensión |
| Ventana SDL + VSync | ✅ Funcional | Sí |
| Delta time, FPS, tiempo total (`Time`) | ✅ Funcional | Sí |
| Logging con niveles (`Log` + macros) | ✅ Funcional | Sí |
| Estado de teclado y ratón (`Input`) | ⚠️ Parcial | Solo "está pulsado", no "se acaba de pulsar" |
| Interfaz de renderizado (`IRenderer2D`) | ⚠️ Esqueleto | Definida pero sin métodos de dibujo |
| Backend SDL de renderizado (`SDLRenderer2D`) | ⚠️ Desconectado | Implementado pero no usado en el bucle |

**Fortalezas del diseño actual:**
- SDL3 no aparece en ningún header público (forward declarations). La API del
  motor es limpia para el consumidor.
- La separación `engine/` vs `game/` es correcta desde el inicio.
- `IRenderer2D` es el punto de extensión correcto para backends futuros.

---

## 2. ¿Qué falta?

### 2.1 Correcciones urgentes (bloquean todo lo demás)

1. **Punto de extensión para el juego.** `Application::OnUpdate` es privado
   y vacío. Sin esto, el código del juego (`game/main.cpp`) no puede hacer nada.
   Solución: clase `Game` virtual o callback registrable.

2. **`IsKeyJustPressed` / `IsKeyJustReleased`.** Sin estado del frame anterior
   es imposible escribir lógica de juego real (saltar, disparar, interactuar).

3. **Conectar `IRenderer2D` al bucle.** `Window::OnUpdate()` hace el render
   directamente. `SDLRenderer2D` existe pero no se usa.

### 2.2 Sistemas que faltan completamente

**Renderizado:**
- Carga y dibujo de texturas/sprites
- Cámara 2D (pan, zoom, follow, screen shake)
- Dibujo de formas geométricas (rect, línea, círculo)
- Texto (font rendering)
- Sprite batching
- Z-ordering y capas de render

**Lógica del juego:**
- Sistema de entidades (Scene Graph o ECS)
- Transform 2D con jerarquía padre-hijo
- Sistema de componentes
- Scene / SceneManager

**Recursos:**
- Resource Manager (caché de activos)
- Carga de texturas (PNG, BMP, etc.)
- Carga de audio

**Físicas:**
- Colisión AABB / Círculo
- Resolución de colisiones
- Triggers

**Audio:**
- Reproducción de sonido (efectos)
- Música de fondo

**Tiempo:**
- Fixed timestep con acumulador
- Cap de deltaTime
- TimeScale

---

## 3. ¿Qué sigue? — Fases de desarrollo

Cada fase produce algo jugable o demostrable al terminar.

---

### FASE 0 — Correcciones base
**Objetivo:** el motor acepta código de juego y tiene input completo.

**0.1 — Punto de extensión para el juego**

Crear `engine/include/Core/Game.h`:
```cpp
namespace EntityEngine {
    class Game {
    public:
        virtual ~Game() = default;
        virtual void OnStart() {}
        virtual void OnUpdate(float deltaTime) {}
        virtual void OnShutdown() {}
    };
}
```

`Application::Run()` recibe un `Game&` o un `unique_ptr<Game>` y llama a sus
métodos en el momento correcto del bucle. El `game/main.cpp` crea una subclase:

```cpp
class MyGame : public EntityEngine::Game {
    void OnStart() override { EE_LOG_INFO("Juego iniciado"); }
    void OnUpdate(float dt) override {
        if (EntityEngine::Input::IsKeyJustPressed(SDL_SCANCODE_ESCAPE))
            // salir...
    }
};
```

**0.2 — Completar `Input` con estado de frame anterior**

En `Input.cpp` añadir `s_PrevKeys[]` y `s_PrevMouseButtons[]`.
Al final de cada frame (nuevo método `Input::EndFrame()`) copiar current a prev.
Añadir:
```cpp
static bool IsKeyJustPressed(SDL_Scancode key);
static bool IsKeyJustReleased(SDL_Scancode key);
static bool IsMouseButtonJustPressed(uint8_t button);
static bool IsMouseButtonJustReleased(uint8_t button);
```

**0.3 — Conectar `IRenderer2D` al bucle**

`Application` crea `std::unique_ptr<IRenderer2D>` con el `SDL_Renderer` de
`Window`. El flujo de `Window::OnUpdate()` se parte:
- `BeginFrame()` llama al `clear`.
- `EndFrame()` llama al `present`.
- `Window` solo expone `GetRenderer()` y no hace render directamente.

**0.4 — Cap de deltaTime en `Time`**

En `Time::Update()`, limitar el deltaTime máximo a ~50ms (20 FPS equivalente)
para evitar spikes que rompan la física futura:
```cpp
s_DeltaTime = std::min(frameTime.count(), 0.05f);
```

**0.5 — Corregir el include en `game/main.cpp`**

Cambiar `#include "../../engine/include/Core/Application.h"` por
`#include "Core/Application.h"` (ya funciona porque `engine/include` está en
`target_include_directories`).

---

### FASE 1 — Texturas y sprites
**Objetivo:** poder cargar una imagen y dibujarla en pantalla.

**1.1 — `Texture`**
Wrappea `SDL_Texture*`. Almacena `int width`, `int height`.
Constructor privado; solo el `ResourceManager` puede crearla.

**1.2 — `ResourceManager`**
Singleton o accedido por referencia. Interfaz clave:
```cpp
std::shared_ptr<Texture> Load(const std::string& path);
void Unload(const std::string& path);
```
Internamente: `unordered_map<string, weak_ptr<Texture>>`.
Si la `weak_ptr` está expirada, recarga. Si está viva, devuelve sin recargar.
Usa SDL_image (`SDL_LoadTexture`) para PNG, JPG, BMP.

**1.3 — Ampliar `IRenderer2D`**
```cpp
virtual void DrawSprite(const Texture& tex,
                        const Rect& src,  // región de la textura
                        const Rect& dst,  // posición y tamaño en pantalla
                        float rotation = 0.0f,
                        Color tint = {255,255,255,255}) = 0;
virtual void DrawRect(const Rect& rect, Color color, bool filled = false) = 0;
virtual void DrawLine(int x0, int y0, int x1, int y1, Color color) = 0;
```

**1.4 — `SDLRenderer2D` implementa los nuevos métodos**
- `DrawSprite` → `SDL_RenderTextureRotated`
- `DrawRect` → `SDL_RenderFillRect` / `SDL_RenderRect`
- `DrawLine` → `SDL_RenderLine`

---

### FASE 2 — Cámara 2D
**Objetivo:** el mundo puede ser más grande que la pantalla.

**2.1 — `Camera2D`**
```cpp
struct Camera2D {
    glm::vec2 position;   // punto del mundo centrado en pantalla
    float zoom = 1.0f;
    float rotation = 0.0f;
};
```

**2.2 — `IRenderer2D` acepta transformación de cámara**
```cpp
virtual void SetCamera(const Camera2D& cam) = 0;
virtual Camera2D GetCamera() const = 0;

// Para dibujar cosas en espacio de pantalla (UI), ignorando la cámara:
virtual void BeginScreenSpace() = 0;
virtual void EndScreenSpace() = 0;
```

**2.3 — Transformación mundo → pantalla en `SDLRenderer2D`**
En SDL3: `SDL_SetRenderScale(renderer, zoom, zoom)` + offset por posición de cámara.

**2.4 — `ScreenToWorld` y `WorldToScreen` helpers**
Necesarios para mouse picking (saber sobre qué entidad del mundo hizo click el usuario).

---

### FASE 3 — Sistema de entidades (Scene Graph)
**Objetivo:** estructurar el juego en entidades con componentes y jerarquía.

**3.1 — `Transform2D`**
```cpp
struct Transform2D {
    glm::vec2 position = {0, 0};
    float rotation = 0.0f;
    glm::vec2 scale = {1, 1};

    glm::mat3 ToMatrix() const;
    static Transform2D Combine(const Transform2D& parent, const Transform2D& child);
};
```

**3.2 — `Component` (base)**
```cpp
class Component {
public:
    Entity* owner = nullptr;
    virtual void OnStart() {}
    virtual void OnUpdate(float dt) {}
    virtual void OnRender(IRenderer2D& renderer) {}
    virtual void OnDestroy() {}
};
```

**3.3 — `Entity` (nodo del scene graph)**
```cpp
class Entity {
public:
    std::string name;
    Transform2D localTransform;
    bool active = true;

    void AddComponent(std::unique_ptr<Component> comp);
    template<typename T> T* GetComponent();

    void AddChild(std::unique_ptr<Entity> child);
    void RemoveChild(Entity* child);

    glm::mat3 GetWorldTransform() const;  // dirty flag internamente

    void Update(float dt);
    void Render(IRenderer2D& renderer);

private:
    Entity* m_Parent = nullptr;
    std::vector<std::unique_ptr<Entity>> m_Children;
    std::vector<std::unique_ptr<Component>> m_Components;
    mutable bool m_DirtyTransform = true;
    mutable glm::mat3 m_CachedWorldTransform;
};
```

**3.4 — `Scene`**
```cpp
class Scene {
public:
    std::string name;
    void Update(float dt);
    void Render(IRenderer2D& renderer);
    Entity* CreateEntity(const std::string& name);
    void DestroyEntity(Entity* entity);

private:
    std::vector<std::unique_ptr<Entity>> m_RootEntities;
};
```

**3.5 — `SceneManager`**
```cpp
class SceneManager {
public:
    void LoadScene(std::unique_ptr<Scene> scene);
    void PushScene(std::unique_ptr<Scene> scene);  // para menú de pausa
    void PopScene();
    Scene* GetActiveScene();
    void Update(float dt);
    void Render(IRenderer2D& renderer);
};
```

**3.6 — Componentes iniciales**
- `SpriteComponent`: textura + rect fuente + color tint.
- `CameraComponent`: define la cámara activa de la escena.
- `ScriptComponent`: permite adjuntar lógica via subclase o lambda.

---

### FASE 4 — Fixed timestep
**Objetivo:** comportamiento determinista para física.

Modificar `Application::Run()`:
```cpp
const float FIXED_DT = 1.0f / 120.0f;
float accumulator = 0.0f;

while (m_IsRunning) {
    Time::Update();
    float dt = Time::GetDeltaTime();  // ya tiene cap en 50ms (Fase 0.4)

    accumulator += dt;
    while (accumulator >= FIXED_DT) {
        game->OnFixedUpdate(FIXED_DT);  // física, colisiones
        accumulator -= FIXED_DT;
    }

    float alpha = accumulator / FIXED_DT;  // para interpolación de render
    game->OnUpdate(dt);
    renderer->BeginFrame();
    sceneManager.Render(*renderer, alpha);
    renderer->EndFrame();
}
```

Añadir `OnFixedUpdate(float fixedDt)` a la clase `Game` base.

---

### FASE 5 — Animación de sprites
**Objetivo:** sprites animados con máquina de estados básica.

**5.1 — `Animation`**
```cpp
struct Animation {
    std::string name;
    std::vector<Rect> frames;
    float frameDuration;
    bool loop = true;
};
```

**5.2 — `AnimatorComponent`**
Hereda de `Component`. Actualiza el frame según el tiempo en `OnUpdate(dt)`.
Expone `SetAnimation(name)` para cambiar el estado activo.

**5.3 — `SpriteSheet`**
Helper para generar automáticamente las rects de un sprite sheet en cuadrícula
dado el tamaño de frame y el número de columnas/filas.

---

### FASE 6 — Colisiones 2D
**Objetivo:** detectar y responder colisiones entre entidades.

**6.1 — Formas de colisión**
```cpp
struct AABB { glm::vec2 min, max; };
struct Circle { glm::vec2 center; float radius; };
```

**6.2 — `ColliderComponent`**
Wrappea la forma. Puede ser trigger (detecta sin resolver).

**6.3 — `PhysicsWorld`**
Actualizado en `OnFixedUpdate`. Pasos:
1. Recoger todos los `ColliderComponent` activos.
2. Broad phase: spatial hash grid.
3. Narrow phase: AABB vs AABB, Círculo vs Círculo, AABB vs Círculo.
4. Resolución: MTV para colisiones sólidas.
5. Callbacks: `Component::OnCollisionEnter(Entity* other)`.

**6.4 — `RigidbodyComponent`**
Velocidad, masa, gravedad. Actualizado en `FixedUpdate`.

---

### FASE 7 — Audio
**Objetivo:** reproducir efectos de sonido y música.

Usar **miniaudio** (header-only, sin dependencias adicionales).

**7.1 — `AudioSystem`**
Inicializado en `Application`. Gestiona el audio device.

**7.2 — `AudioClip`**
Buffer PCM decodificado. Cargado por `ResourceManager`.

**7.3 — API de alto nivel**
```cpp
AudioSystem::PlaySound(clip, volume, pitch);
AudioSystem::PlayMusic(clip, loop, volume);
AudioSystem::SetMusicVolume(float);
AudioSystem::StopAll();
```

---

### FASE 8 — Texto y fuentes
**Objetivo:** renderizar texto en pantalla.

Usar **stb_truetype** (header-only) para generar atlas de fuente.

**8.1 — `Font`**
Genera un atlas de textura con todos los glifos de una fuente TTF a un tamaño.

**8.2 — `IRenderer2D::DrawText`**
```cpp
virtual void DrawText(const Font& font, const std::string& text,
                      glm::vec2 position, Color color, float scale = 1.0f) = 0;
```

---

## 4. ¿Qué hay que tener en cuenta?

### 4.1 Adaptabilidad futura: la regla de oro

**Ningún header en `engine/include/` debe incluir `<SDL3/SDL.h>` ni ningún
header de una API gráfica concreta.** Los tipos SDL deben quedar como forward
declarations o dentro de los `.cpp`.

Esta regla ya se cumple en `Window.h` (usa `struct SDL_Window;`) y debe
mantenerse en todos los sistemas nuevos.

---

### 4.2 `IRenderer2D` debe usar handles, no punteros a objetos SDL

La interfaz actual trabaja con referencias a `Texture`. Para que OpenGL o Vulkan
puedan implementarla, la `Texture` no puede contener `SDL_Texture*` dentro.

Solución: **handles opacos**.

```cpp
using TextureHandle = uint32_t;  // solo un entero
static constexpr TextureHandle INVALID_TEXTURE = 0;

class IRenderer2D {
    // El renderer crea y destruye texturas internamente
    virtual TextureHandle CreateTexture(int w, int h, const void* pixelData) = 0;
    virtual void DestroyTexture(TextureHandle handle) = 0;
    virtual void DrawSprite(TextureHandle tex, ...) = 0;
};
```

El `SDLRenderer` guarda internamente `unordered_map<uint32_t, SDL_Texture*>`.  
El `OpenGLRenderer` guarda `unordered_map<uint32_t, GLuint>`.  
El `VulkanRenderer` guarda `unordered_map<uint32_t, VkImage>`.  
El código del juego nunca sabe qué hay dentro.

---

### 4.3 La ventana debe separarse del renderer cuando llegue el momento

Actualmente `Window` crea el `SDL_Renderer` internamente. En OpenGL la ventana
crea el contexto GL. En Vulkan la ventana solo provee una `VkSurfaceKHR` y el
Vulkan renderer crea su propia swapchain.

**No es necesario refactorizar esto ahora.** Pero cuando se implemente el segundo
backend (OpenGL), la refactorización será:

```
Window → solo gestiona la ventana del SO y expone:
    - GetNativeWindowHandle() → void* (SDL_Window*, HWND, etc.)
    - GetDrawableSize() → ivec2

RenderContext → se construye sobre Window:
    - SDLRenderContext  → crea SDL_Renderer
    - OpenGLContext     → crea SDL_GL_Context
    - VulkanContext     → crea VkInstance + VkSurface + Swapchain

IRenderer2D → se construye sobre RenderContext
```

---

### 4.4 Ruta recomendada hacia OpenGL y Vulkan

Completar el motor 2D completamente antes de introducir un segundo backend.
Cambiar el backend sin tener el motor funcional genera complejidad prematura.

**Orden sugerido:**

1. Motor 2D completo con SDL (Fases 0–8).
2. Refactorizar `Window` para separar contexto de render.
3. Implementar `OpenGLRenderer2D` como segundo backend.
   - Valida que la abstracción es correcta y no tiene filtraciones de SDL.
   - Introduce shaders. Añadir `ShaderHandle` a la interfaz.
4. Motor 3D con OpenGL: Transform3D, Camera3D, Mesh, Material, Light.
5. Implementar `VulkanRenderer` una vez OpenGL esté dominado.
   - Vulkan es 10× más verboso pero más eficiente y portátil.

---

### 4.5 Dependencias externas recomendadas

| Necesidad | Librería | Por qué |
|---|---|---|
| Math (vec2, mat3, mat4) | **glm** | Estándar de facto, header-only, OpenGL-compatible |
| Carga de imágenes | **SDL_image** (corto plazo) / **stb_image** (largo plazo) | stb_image es header-only y no depende de SDL |
| Texto / fuentes | **stb_truetype** | Header-only, funciona con cualquier backend |
| Audio | **miniaudio** | Header-only, sin deps, multiplataforma |
| Física 2D | **Box2D 2.x** | Maduro, bien documentado, abstratable |
| Serialización JSON | **nlohmann/json** | Header-only, muy usado en C++ moderno |
| ECS (futuro) | **EnTT** | Header-only, archetype-based, excelente rendimiento |

**Principio:** preferir librerías header-only para dependencias menores (stb,
miniaudio, glm, nlohmann). Para dependencias mayores (SDL3, Box2D, EnTT) usar
como submódulos de git o paquetes de sistema.

---

### 4.6 Consideraciones de estructura para el editor futuro

Los motores maduros tienen un editor separado que consume el motor. Para que
EntityEngine pueda tener un editor en el futuro:

- Los sistemas (`PhysicsWorld`, `SceneManager`, `AudioSystem`) deben ser
  instanciables y configurables externamente, no singletons globales.
- La serialización de escenas (Fase 3) debe definirse desde el inicio con el
  editor en mente (formato JSON con campo `type` por componente).
- El motor no debe suponer que la aplicación es siempre un juego: el editor
  puede necesitar múltiples vistas, viewport dividido, etc.

Esto no bloquea el desarrollo actual, pero tenerlo en mente evita decisiones
de diseño que dificulten el editor más adelante.

---

### 4.7 Resumen de decisiones tomadas y por tomar

| Decisión | Estado | Opción elegida |
|---|---|---|
| API gráfica primaria | ✅ Decidido | SDL3 (2D), OpenGL después |
| Lenguaje | ✅ Decidido | C++23 |
| Build system | ✅ Decidido | CMake |
| Tipo de entidades | ⏳ Pendiente Fase 3 | Scene Graph recomendado |
| Física 2D | ⏳ Pendiente Fase 6 | Box2D 2.x recomendado |
| Audio | ⏳ Pendiente Fase 7 | miniaudio recomendado |
| Formato de escena | ⏳ Pendiente Fase 3 | JSON (nlohmann) recomendado |
| Scripting | ⏳ Futuro lejano | Lua o GDScript-like |

---

*Última actualización: 2026-05-06*
