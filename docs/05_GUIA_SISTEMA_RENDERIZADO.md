# Guía para implementar el Sistema de Renderizado

Esta guía explica qué es un sistema de renderizado, por qué existe la abstracción
`IRenderer2D` en EntityEngine, cuál es el objetivo del sistema y cómo desarrollarlo
paso a paso desde cero.

---

## ÍNDICE

1. [¿Qué hace un sistema de renderizado?](#1-qué-hace-un-sistema-de-renderizado)
2. [Estado actual del motor](#2-estado-actual-del-motor)
3. [Por qué existe la abstracción IRenderer2D](#3-por-qué-existe-la-abstracción-irenderer2d)
4. [El ciclo de un frame de renderizado](#4-el-ciclo-de-un-frame-de-renderizado)
5. [Fase 0.3 — Conectar IRenderer2D al bucle](#5-fase-03--conectar-irenderer2d-al-bucle)
6. [Fase 1 — Primitivas y formas básicas](#6-fase-1--primitivas-y-formas-básicas)
7. [Fase 2 — Texturas y sprites](#7-fase-2--texturas-y-sprites)
8. [Fase 3 — Handles opacos](#8-fase-3--handles-opacos)
9. [Fase 4 — ResourceManager](#9-fase-4--resourcemanager)
10. [Fase 5 — Cámara 2D](#10-fase-5--cámara-2d)
11. [Reglas que debe respetar el sistema](#11-reglas-que-debe-respetar-el-sistema)
12. [Orden de implementación recomendado](#12-orden-de-implementación-recomendado)

---

## 1. ¿Qué hace un sistema de renderizado?

Un sistema de renderizado es el subsistema del motor responsable de producir
la imagen que el jugador ve en pantalla cada frame.

A muy alto nivel, su trabajo es:

```txt
Estado del mundo (posición de entidades, texturas, cámara)
                       ↓
              Sistema de renderizado
                       ↓
              Imagen en pantalla
```

Eso incluye tareas como:

- Limpiar la pantalla con un color de fondo al inicio de cada frame.
- Dibujar rectángulos, líneas y formas geométricas.
- Cargar imágenes desde disco y dibujarlas como sprites.
- Transformar coordenadas del mundo a coordenadas de pantalla (cámara).
- Enviar la imagen terminada al monitor (present/swap).

El sistema de renderizado **no decide qué dibujar**. Eso lo decide el juego.
El sistema solo provee las herramientas para hacerlo.

### Por qué no se dibuja directamente con SDL

SDL3 tiene funciones como `SDL_RenderFillRect` y `SDL_RenderTexture` que
dibujan directamente. En un proyecto pequeño está bien usarlas directo.

En un motor, el problema es que si el código del juego llama a funciones SDL
directamente, el motor queda atado a SDL para siempre. Cambiar a OpenGL o
Vulkan requeriría reescribir todo el código del juego.

La solución es una **interfaz abstracta**: el juego solo llama a métodos del
motor (`DrawSprite`, `DrawRect`). El motor decide internamente cómo traducir
eso a SDL, OpenGL, Vulkan, etc.

---

## 2. Estado actual del motor

EntityEngine tiene tres piezas de renderizado, pero solo una está activa:

| Archivo | Estado | Qué hace |
|---|---|---|
| `engine/include/Render/IRenderer2D.h` | Esqueleto | Interfaz abstracta con solo `BeginFrame`, `EndFrame`, `Clear` |
| `engine/include/Render/Backend/SDLRenderer2D.h` | Implementado | Implementa la interfaz usando SDL3 |
| `engine/src/Platform/SDLWindow.cpp` | Activo | Hace el clear y present directamente, sin usar `IRenderer2D` |

El problema central es que `SDLRenderer2D` existe pero **no se usa**. El
bucle principal en `Application` llama a `Window::OnUpdate()`, que hace el
render directamente con SDL sin pasar por `IRenderer2D`.

Esto significa que toda la arquitectura de abstracción está preparada pero
desconectada.

---

## 3. Por qué existe la abstracción IRenderer2D

### La analogía del enchufe

Imagina que un electrodoméstico tuviera el cable soldado directamente a la
pared. Para cambiarlo de habitación habría que romper paredes.

Los enchufes existen para separar el electrodoméstico de la fuente de energía.
El electrodoméstico no sabe si la corriente viene de la red, de un generador
o de un panel solar. Solo sabe que enchufando en la interfaz estándar funciona.

`IRenderer2D` es el enchufe del motor. El juego conecta ahí. El motor puede
cambiar el backend (SDL, OpenGL, Vulkan) sin que el juego lo note.

### En código

Sin abstracción, el juego hace esto:

```cpp
SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
SDL_RenderFillRect(renderer, &rect);
```

Con abstracción, el juego hace esto:

```cpp
renderer.DrawRect({x, y, w, h}, {255, 0, 0, 255}, true);
```

El método `DrawRect` existe en `IRenderer2D`. La implementación concreta
(`SDLRenderer2D`, `OpenGLRenderer2D`, etc.) decide cómo ejecutarlo internamente.

### Diseño actual de IRenderer2D

```cpp
class IRenderer2D
{
public:
    virtual ~IRenderer2D() = default;

    virtual void BeginFrame() = 0;
    virtual void EndFrame()   = 0;
    virtual void Clear(const Color& color) = 0;
};
```

Este es solo el esqueleto del ciclo de frame. Faltan todos los métodos de
dibujo real: sprites, rectángulos, líneas, texto.

---

## 4. El ciclo de un frame de renderizado

Cada frame, el sistema de renderizado sigue esta secuencia fija:

```txt
1. BeginFrame()    ← preparar el renderer para recibir draw calls
2. Clear(color)    ← limpiar la pantalla con el color de fondo
3. DrawXxx(...)    ← todas las llamadas de dibujo del juego
4. EndFrame()      ← enviar la imagen terminada al monitor (SDL_RenderPresent)
```

Esto es análogo a pintar un cuadro:

- `BeginFrame`: preparar el lienzo.
- `Clear`: pintar el lienzo de un color base.
- `DrawXxx`: añadir todos los elementos encima.
- `EndFrame`: mostrar el cuadro terminado.

### Por qué `BeginFrame` y `EndFrame` son importantes

En SDL3 el renderer es una máquina de estados. Algunas operaciones deben
ocurrir antes de cualquier draw call y otras deben ocurrir después de todas.
`BeginFrame` y `EndFrame` son los puntos de entrada y salida que garantizan
ese orden.

En OpenGL/Vulkan la importancia es mayor: hay command buffers, render passes y
sincronización explícita que deben abrirse y cerrarse correctamente.

---

## 5. Fase 0.3 — Conectar IRenderer2D al bucle

Este es el primer paso real a implementar. Es el más importante porque
desbloquea todo lo demás.

### El problema actual

`Window::OnUpdate()` hace el clear y present directamente:

```cpp
void Window::OnUpdate()
{
    SDL_SetRenderDrawColor(m_Renderer, 20, 20, 25, 255);
    SDL_RenderClear(m_Renderer);
    // sin draw calls reales
    SDL_RenderPresent(m_Renderer);
}
```

`Application` llama `m_Window->OnUpdate()` cada frame. El `SDLRenderer2D`
nunca se usa.

### Lo que hay que hacer

**Paso 1: `Application` crea y posee el renderer.**

En `Application.h`, añadir:

```cpp
#include "Render/IRenderer2D.h"
#include <memory>

class Application
{
private:
    std::unique_ptr<IRenderer2D> m_Renderer;
    // ... resto de miembros existentes
};
```

**Paso 2: `Application` inicializa el renderer con el SDL_Renderer de Window.**

En `Application.cpp`, en el constructor, después de crear la ventana:

```cpp
// Window ya tiene SDL_Renderer creado internamente
// SDLRenderer2D no posee el renderer, solo lo usa
m_Renderer = std::make_unique<SDLRenderer2D>(m_Window->GetRenderer());
```

**Paso 3: `Window::OnUpdate()` se parte en dos responsabilidades.**

`Window` solo debe gestionar la ventana (tamaño, título, handle nativo).
El render debe ir por `IRenderer2D`.

Resultado: `Window::OnUpdate()` queda vacío o se elimina.

**Paso 4: El bucle de `Application::Run()` usa el renderer.**

```cpp
void Application::Run()
{
    while (m_IsRunning)
    {
        Time::Update();

        // Procesar eventos
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            Input::OnEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                m_IsRunning = false;
        }

        float dt = Time::GetDeltaTime();
        OnUpdate(dt);

        // Ciclo de render
        m_Renderer->Clear({20, 20, 25, 255});
        m_Renderer->BeginFrame();
        // Aquí irán los draw calls cuando existan
        m_Renderer->EndFrame();

        Input::EndFrame();
    }
}
```

### Qué se consigue con este paso

- `SDLRenderer2D` pasa a ser la pieza que produce la imagen.
- `Window` queda limpia: solo gestiona ventana, no render.
- El juego puede acceder a `IRenderer2D` para dibujar (cuando exista el
  punto de extensión `Game`).
- Añadir un backend OpenGL en el futuro solo requiere crear un
  `OpenGLRenderer2D` y pasarlo en el constructor.

---

## 6. Fase 1 — Primitivas y formas básicas

Antes de cargar imágenes, es útil poder dibujar formas simples. Sirven para
debug, prototipar físicas y entender el sistema antes de añadir complejidad.

### Qué hay que añadir a IRenderer2D

```cpp
// Un rectángulo con posición y tamaño
struct Rect
{
    float x, y, w, h;
};

class IRenderer2D
{
public:
    // ... BeginFrame, EndFrame, Clear (ya existen)

    // Relleno o borde según el parámetro filled
    virtual void DrawRect(const Rect& rect, Color color, bool filled = true) = 0;

    // Línea entre dos puntos
    virtual void DrawLine(float x0, float y0, float x1, float y1, Color color) = 0;

    // Círculo aproximado con segmentos de línea
    virtual void DrawCircle(float cx, float cy, float radius, Color color) = 0;
};
```

### Implementación en SDLRenderer2D

`SDL_RenderFillRect` dibuja un rectángulo relleno. `SDL_RenderRect` dibuja
solo el borde. `SDL_RenderLine` dibuja una línea.

Para el círculo, SDL3 no tiene una función nativa. Se aproxima dibujando
`N` segmentos de línea alrededor del centro:

```cpp
void SDLRenderer2D::DrawCircle(float cx, float cy, float radius, Color color)
{
    SDL_SetRenderDrawColor(m_Renderer, color.r, color.g, color.b, color.a);

    const int segments = 32;
    for (int i = 0; i < segments; ++i)
    {
        float angle0 = (float)i / segments * 2.0f * M_PI;
        float angle1 = (float)(i + 1) / segments * 2.0f * M_PI;

        float x0 = cx + std::cos(angle0) * radius;
        float y0 = cy + std::sin(angle0) * radius;
        float x1 = cx + std::cos(angle1) * radius;
        float y1 = cy + std::sin(angle1) * radius;

        SDL_RenderLine(m_Renderer, x0, y0, x1, y1);
    }
}
```

### Por qué es útil esto ahora

Con solo formas puedes:

- Ver los hitboxes de los personajes (cuadrados de colisión).
- Depurar posiciones de entidades sin texturas.
- Probar el sistema de cámara con objetos simples.

---

## 7. Fase 2 — Texturas y sprites

Esta es la fase principal del renderizado visible. Permite cargar imágenes
y dibujarlas en pantalla.

### Qué es un sprite

Un sprite es una imagen 2D que se dibuja en una posición del mundo.

En la mayoría de motores, un sprite tiene:

- Una textura (la imagen en memoria de GPU).
- Un rectángulo fuente (`srcRect`): qué parte de la textura usar (útil para sprite sheets).
- Un rectángulo destino (`dstRect`): dónde y con qué tamaño dibujarlo en pantalla.

```txt
Textura completa (512x512 píxeles)
┌─────────────────────────────────────┐
│                                     │
│  ┌──────┐  srcRect (32x32)           │
│  │ Img  │  La región que usamos     │
│  └──────┘                           │
│                                     │
└─────────────────────────────────────┘
            ↓ se dibuja en
┌──────────────────────┐
│ Pantalla             │
│   ┌──────────────┐   │
│   │   dstRect    │   │  La posición y tamaño en pantalla
│   │  (64x64 en   │   │  puede ser diferente al original
│   │   pantalla)  │   │
│   └──────────────┘   │
└──────────────────────┘
```

### Qué hay que añadir a IRenderer2D

```cpp
// Handle opaco a una textura (solo un número entero)
using TextureHandle = uint32_t;
static constexpr TextureHandle INVALID_TEXTURE = 0;

class IRenderer2D
{
public:
    // ... BeginFrame, EndFrame, Clear, DrawRect, DrawLine, DrawCircle

    // Crear una textura a partir de datos en memoria (píxeles RGBA)
    virtual TextureHandle CreateTexture(int width, int height,
                                        const void* pixelData) = 0;

    // Liberar una textura cuando ya no se necesita
    virtual void DestroyTexture(TextureHandle handle) = 0;

    // Dibujar un sprite
    virtual void DrawSprite(TextureHandle texture,
                            const Rect& src,
                            const Rect& dst,
                            float rotation = 0.0f,
                            Color tint = {255, 255, 255, 255}) = 0;
};
```

### Por qué TextureHandle es un número entero y no un puntero

Si `DrawSprite` recibiera un `SDL_Texture*`, la interfaz ya no sería abstracta.
Un `OpenGLRenderer2D` no tiene `SDL_Texture`.

Con un entero (`TextureHandle`), la interfaz es genérica. Cada backend mantiene
internamente una tabla que traduce ese entero a su objeto real:

```txt
SDLRenderer2D internamente:
  unordered_map<uint32_t, SDL_Texture*>

OpenGLRenderer2D internamente:
  unordered_map<uint32_t, GLuint>

VulkanRenderer internamente:
  unordered_map<uint32_t, VkImage>
```

El juego nunca sabe qué hay dentro. Solo usa el número.

### Implementación en SDLRenderer2D

`SDLRenderer2D` necesita un mapa y un contador para generar handles únicos:

```cpp
class SDLRenderer2D : public IRenderer2D
{
private:
    SDL_Renderer* m_Renderer;
    std::unordered_map<uint32_t, SDL_Texture*> m_Textures;
    uint32_t m_NextHandle = 1;  // 0 es INVALID_TEXTURE
};
```

`CreateTexture` crea la textura SDL y devuelve un handle:

```cpp
TextureHandle SDLRenderer2D::CreateTexture(int w, int h, const void* pixelData)
{
    // SDL_CreateTexture crea la textura en la GPU
    SDL_Texture* tex = SDL_CreateTexture(
        m_Renderer,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STATIC,
        w, h
    );

    if (!tex) return INVALID_TEXTURE;

    // Subir los píxeles a la GPU
    SDL_UpdateTexture(tex, nullptr, pixelData, w * 4);

    uint32_t handle = m_NextHandle++;
    m_Textures[handle] = tex;
    return handle;
}
```

`DestroyTexture` libera la textura de la GPU:

```cpp
void SDLRenderer2D::DestroyTexture(TextureHandle handle)
{
    auto it = m_Textures.find(handle);
    if (it != m_Textures.end())
    {
        SDL_DestroyTexture(it->second);
        m_Textures.erase(it);
    }
}
```

`DrawSprite` traduce el handle a `SDL_Texture*` y llama a SDL:

```cpp
void SDLRenderer2D::DrawSprite(TextureHandle handle,
                               const Rect& src,
                               const Rect& dst,
                               float rotation,
                               Color tint)
{
    auto it = m_Textures.find(handle);
    if (it == m_Textures.end()) return;

    SDL_FRect sdlSrc {src.x, src.y, src.w, src.h};
    SDL_FRect sdlDst {dst.x, dst.y, dst.w, dst.h};

    SDL_SetTextureColorMod(it->second, tint.r, tint.g, tint.b);
    SDL_SetTextureAlphaMod(it->second, tint.a);

    SDL_RenderTextureRotated(m_Renderer, it->second,
                              &sdlSrc, &sdlDst,
                              rotation, nullptr, SDL_FLIP_NONE);
}
```

---

## 8. Fase 3 — Handles opacos

Este concepto ya se introdujo en la sección anterior pero merece explicarse
por separado porque es una de las decisiones de arquitectura más importantes
del motor.

### El problema sin handles

Si `Texture` fuera una clase que contiene `SDL_Texture*` internamente:

```cpp
class Texture
{
public:
    SDL_Texture* m_SdlTexture;  // ← SDL se filtra a la interfaz
    int width, height;
};
```

Para que `IRenderer2D::DrawSprite` funcione con OpenGL o Vulkan, habría que
cambiar `Texture` para que tenga `GLuint` o `VkImage` también. La clase se
llenaría de miembros condicionales por backend y la abstracción se rompería.

### La solución: handles

Un handle es solo un número entero que identifica un recurso. No contiene
datos internos del backend.

```cpp
using TextureHandle = uint32_t;
```

El recurso real (SDL_Texture, GLuint, VkImage) vive dentro del renderer, en
una tabla privada. Solo el renderer sabe cómo traducir ese número.

### La regla

Ningún tipo expuesto en `engine/include/Render/IRenderer2D.h` puede incluir
headers de SDL, OpenGL o Vulkan.

Solo pueden aparecer:

- Tipos primitivos (`uint32_t`, `float`, `int`).
- Tipos propios del motor (`Color`, `Rect`, `TextureHandle`).
- Clases abstractas del motor (`IRenderer2D`).

---

## 9. Fase 4 — ResourceManager

El `ResourceManager` es el sistema que carga activos desde disco y los
entrega al juego como handles.

### Por qué es necesario

Sin `ResourceManager`, el juego haría esto:

```cpp
// Cargar la imagen desde disco
auto pixelData = LoadPNG("player.png");  // ¿cómo? ¿con qué librería?
TextureHandle handle = renderer.CreateTexture(64, 64, pixelData.data());
```

Esto tiene problemas:

- Si dos sistemas cargan la misma imagen, se subirían dos copias a la GPU.
- La imagen se cargaría de disco cada vez.
- Liberar la textura manualmente es propenso a errores.

Con `ResourceManager`:

```cpp
TextureHandle handle = resources.Load("player.png");
// Si ya estaba cargada, devuelve el mismo handle
// Si no, la carga, sube a GPU y la cachea
```

### Estructura básica

```cpp
class ResourceManager
{
public:
    // Carga una textura desde disco o devuelve la cacheada
    TextureHandle Load(const std::string& path);

    // Libera explícitamente (opcional, puede hacerse automático)
    void Unload(const std::string& path);

private:
    IRenderer2D* m_Renderer;
    std::unordered_map<std::string, TextureHandle> m_Cache;
};
```

### Carga de imágenes con stb_image

Para cargar PNG, JPG, BMP sin depender de SDL_image, se usa `stb_image`.
Es una librería header-only: un solo archivo `.h` que se incluye en el proyecto.

```cpp
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

TextureHandle ResourceManager::Load(const std::string& path)
{
    // Si ya existe en caché, devolver el handle existente
    auto it = m_Cache.find(path);
    if (it != m_Cache.end())
        return it->second;

    // Cargar la imagen desde disco
    int width, height, channels;
    // stbi_load devuelve puntero a datos RGBA en heap
    uint8_t* data = stbi_load(path.c_str(), &width, &height, &channels, 4);

    if (!data)
    {
        EE_LOG_ERROR("No se pudo cargar textura: " + path);
        return INVALID_TEXTURE;
    }

    // Subir a GPU a través del renderer
    TextureHandle handle = m_Renderer->CreateTexture(width, height, data);

    stbi_image_free(data);  // Liberar memoria CPU

    m_Cache[path] = handle;
    return handle;
}
```

---

## 10. Fase 5 — Cámara 2D

La cámara define qué parte del mundo es visible en pantalla.

### El problema sin cámara

Sin cámara, las coordenadas del juego son directamente píxeles de pantalla.
Si el jugador está en la posición (500, 300) del mundo, se dibuja en el
píxel (500, 300) de la ventana.

Esto funciona si el nivel cabe en pantalla. En cuanto el nivel es más grande,
el jugador desaparece fuera del borde de la pantalla.

### Qué es una cámara 2D

La cámara define un punto del mundo que aparece en el centro de la pantalla,
y un zoom que determina cuánto mundo es visible.

```cpp
struct Camera2D
{
    float x = 0.0f;     // punto del mundo en el centro de pantalla
    float y = 0.0f;
    float zoom = 1.0f;  // 1.0 = normal, 2.0 = doble de zoom (más cerca)
};
```

### La transformación mundo → pantalla

Para saber dónde dibujar un objeto del mundo en pantalla, se aplica la
transformación de cámara:

```txt
screenX = (worldX - camera.x) * zoom + pantalla_ancho / 2
screenY = (worldY - camera.y) * zoom + pantalla_alto / 2
```

Ejemplo:

```txt
Camera en posición (200, 150), zoom 1.0, pantalla 800x600

Objeto del mundo en (220, 160):
  screenX = (220 - 200) * 1.0 + 400 = 420
  screenY = (160 - 150) * 1.0 + 300 = 310

El objeto se dibuja en el píxel (420, 310) de pantalla.
```

Ejemplo con zoom 2.0:

```txt
Objeto del mundo en (220, 160):
  screenX = (220 - 200) * 2.0 + 400 = 440
  screenY = (160 - 150) * 2.0 + 300 = 320

Con más zoom, el objeto aparece más lejos del centro (efecto de acercamiento).
```

### Implementación en IRenderer2D

La cámara se pasa al renderer para que aplique la transformación a todos los
draw calls posteriores:

```cpp
class IRenderer2D
{
public:
    // ... métodos existentes

    virtual void SetCamera(const Camera2D& camera) = 0;
    virtual Camera2D GetCamera() const = 0;

    // Para UI que debe dibujarse en coordenadas de pantalla
    // (salud, puntuación, menús), ignorando la cámara del mundo
    virtual void BeginScreenSpace() = 0;
    virtual void EndScreenSpace() = 0;
};
```

### Implementación en SDLRenderer2D

SDL3 tiene `SDL_SetRenderScale` para aplicar zoom global y `SDL_SetRenderViewport`
para el offset de cámara.

```cpp
void SDLRenderer2D::SetCamera(const Camera2D& camera)
{
    m_Camera = camera;

    // Aplicar zoom
    SDL_SetRenderScale(m_Renderer, camera.zoom, camera.zoom);

    // Calcular offset de la cámara
    // El origen del mundo en pantalla después del zoom
    SDL_FRect viewport = {
        -(camera.x * camera.zoom - m_ScreenWidth * 0.5f),
        -(camera.y * camera.zoom - m_ScreenHeight * 0.5f),
        (float)m_ScreenWidth,
        (float)m_ScreenHeight
    };
    SDL_SetRenderViewport(m_Renderer, nullptr);
}
```

### La transformación inversa (pantalla → mundo)

Necesaria para saber en qué punto del mundo hizo click el jugador:

```cpp
// Convertir una posición de pantalla (ej: posición del ratón)
// a coordenada del mundo
float worldX = (screenX - pantalla_ancho / 2.0f) / zoom + camera.x;
float worldY = (screenY - pantalla_alto / 2.0f) / zoom + camera.y;
```

Se expone como función helper:

```cpp
class IRenderer2D
{
public:
    virtual glm::vec2 ScreenToWorld(glm::vec2 screenPos) const = 0;
    virtual glm::vec2 WorldToScreen(glm::vec2 worldPos) const = 0;
};
```

### Mundo vs espacio de pantalla (UI)

La cámara afecta todo lo que se dibuja. Pero la UI (barra de salud, puntuación,
menús) debe dibujarse en coordenadas fijas de pantalla, no del mundo.

Por eso existen `BeginScreenSpace()` y `EndScreenSpace()`. Entre esas llamadas,
la transformación de cámara se ignora:

```cpp
// Dibujar el mundo (afectado por la cámara)
renderer.SetCamera(gameCamera);
renderer.DrawSprite(playerTexture, src, worldDst);
renderer.DrawSprite(enemyTexture, src, worldDst);

// Dibujar la UI (siempre en la misma posición de pantalla)
renderer.BeginScreenSpace();
renderer.DrawSprite(healthBarTexture, src, {10, 10, 200, 30});
renderer.EndScreenSpace();
```

---

## 11. Reglas que debe respetar el sistema

Estas reglas vienen del plan de acción del proyecto y son críticas para que
el sistema sea extensible en el futuro.

### Regla 1: Ningún header público puede incluir SDL

Todo lo que está en `engine/include/` debe ser legible sin que el proyecto que
lo usa tenga SDL en su include path.

Correcto:

```cpp
// engine/include/Render/IRenderer2D.h
#pragma once
#include <cstdint>
// No hay ningún #include <SDL3/SDL.h>

struct Color { uint8_t r, g, b, a; };
using TextureHandle = uint32_t;

class IRenderer2D { ... };
```

Incorrecto:

```cpp
// engine/include/Render/IRenderer2D.h
#include <SDL3/SDL.h>  // ← rompe la abstracción
```

### Regla 2: Los tipos de la interfaz son agnósticos al backend

`Color`, `Rect`, `TextureHandle` son tipos propios del motor. No son tipos SDL,
OpenGL ni Vulkan.

Correcto:

```cpp
virtual void DrawSprite(TextureHandle tex, const Rect& dst) = 0;
```

Incorrecto:

```cpp
virtual void DrawSprite(SDL_Texture* tex, SDL_FRect dst) = 0;  // ← SDL en la interfaz
```

### Regla 3: El renderer no posee la ventana

`SDLRenderer2D` recibe un `SDL_Renderer*` externo. La ventana (y el renderer SDL)
los posee y gestiona la clase `Window`. El destructor de `SDLRenderer2D` no
debe llamar `SDL_DestroyRenderer`.

Esto ya se cumple en el código actual.

### Regla 4: La vida de las texturas la gestiona el renderer

Una textura creada con `CreateTexture` debe ser destruida con `DestroyTexture`
antes de que el renderer se destruya. El `ResourceManager` es responsable de
esto, no el código del juego directamente.

---

## 12. Orden de implementación recomendado

Siguiendo las fases del plan de acción:

### Paso 1 — Conectar IRenderer2D (Fase 0.3)

Este paso desbloquea todo lo demás.

1. En `Application.h`: añadir `std::unique_ptr<IRenderer2D> m_Renderer`.
2. En `Application.cpp`: construir `SDLRenderer2D` con `m_Window->GetRenderer()`.
3. En `Application::Run()`: llamar `Clear`, `BeginFrame`, `EndFrame` desde el renderer.
4. Vaciar `Window::OnUpdate()` (ya no hace el render).
5. Compilar y verificar que la ventana sigue mostrando el color de fondo.

### Paso 2 — Primitivas (Fase 1 parcial)

1. Añadir `struct Rect` a `IRenderer2D.h`.
2. Añadir `DrawRect`, `DrawLine`, `DrawCircle` a `IRenderer2D`.
3. Implementar los tres métodos en `SDLRenderer2D`.
4. Probar dibujando un rectángulo en `Application::OnUpdate`.

### Paso 3 — TextureHandle y DrawSprite (Fase 1)

1. Añadir `using TextureHandle = uint32_t` y `INVALID_TEXTURE` a `IRenderer2D.h`.
2. Añadir `CreateTexture`, `DestroyTexture`, `DrawSprite` a `IRenderer2D`.
3. En `SDLRenderer2D`: añadir el mapa interno y el contador de handles.
4. Implementar `CreateTexture`, `DestroyTexture`, `DrawSprite`.
5. Probar con datos de píxeles hardcodeados (un cuadrado de color sólido).

### Paso 4 — ResourceManager (Fase 1)

1. Añadir `stb_image.h` al proyecto en `external/`.
2. Crear `engine/include/Resources/ResourceManager.h`.
3. Crear `engine/src/Resources/ResourceManager.cpp`.
4. Implementar `Load` con caché y `Unload`.
5. Probar cargando un PNG real desde disco.

### Paso 5 — Camera2D (Fase 2)

1. Añadir `struct Camera2D` a `IRenderer2D.h`.
2. Añadir `SetCamera`, `GetCamera`, `BeginScreenSpace`, `EndScreenSpace` a `IRenderer2D`.
3. Implementar en `SDLRenderer2D` usando `SDL_SetRenderScale`.
4. Añadir `ScreenToWorld` y `WorldToScreen`.
5. Probar moviendo la cámara con teclas.

---

*Última actualización: 2026-05-07*
