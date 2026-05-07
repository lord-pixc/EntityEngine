# EntityEngine — Documentación técnica del proyecto

> Estado actual del código: qué hace cada sistema, cómo fluyen los datos
> entre ellos y cuáles son las limitaciones conocidas.

---

## ÍNDICE

1. [Visión general](#1-visión-general)
2. [Estructura de archivos](#2-estructura-de-archivos)
3. [Sistemas implementados](#3-sistemas-implementados)
4. [Flujo de datos completo por frame](#4-flujo-de-datos-completo-por-frame)
5. [Limitaciones y deuda técnica conocida](#5-limitaciones-y-deuda-técnica-conocida)

---

## 1. Visión general

EntityEngine es una biblioteca estática C++23 que abstrae SDL3 para gestionar
ventana, tiempo, input y renderizado básico. Se compila como `libEngine.a` y
el ejecutable de ejemplo en `build/game/Game` la consume.

**Stack tecnológico:**
- Lenguaje: C++23
- Sistema de ventana/render: SDL3
- Build system: CMake 3.23+
- SDL3 pre-compilado en `external/SDL3/build/`

**Dos targets CMake:**
- `Engine` → `build/engine/libEngine.a` (biblioteca estática)
- `Game` → `build/game/Game` (ejecutable de demo)

---

## 2. Estructura de archivos

```
EntityEngine/
├── engine/
│   ├── include/           ← API pública del motor (sin headers SDL en la interfaz)
│   │   ├── Core/
│   │   │   ├── Application.h   ← Punto de entrada del motor
│   │   │   ├── Time.h          ← Utilidades de tiempo
│   │   │   └── log.h           ← Sistema de logging
│   │   ├── Platform/
│   │   │   ├── Window.h        ← Abstracción de ventana (SDL ocultado)
│   │   │   └── Input.h        ← Estado de teclado y ratón
│   │   └── Render/
│   │       ├── IRenderer2D.h               ← Interfaz abstracta de render 2D
│   │       └── Backend/
│   │           └── SDLRenderer2D.h         ← Implementación SDL de IRenderer2D
│   └── src/               ← Implementaciones (sí pueden incluir SDL)
│       ├── Core/
│       │   ├── Application.cpp
│       │   ├── Time.cpp
│       │   └── log.cpp
│       ├── Platform/
│       │   ├── Input.cpp
│       │   └── SDLWindow.cpp
│       └── Render/
│           └── SDLRenderer2D.cpp
│                                    
├── game/
│   └── src/
│       └── main.cpp        ← Punto de entrada del ejecutable de demo
└── external/
    └── SDL3/               ← SDL3 pre-compilado (no se toca)
```

---

## 3. Sistemas implementados

### 3.1 `Application` — El bucle principal

**Archivos:** `engine/include/Core/Application.h`, `engine/src/Core/Application.cpp`

**Responsabilidades:**
- Inicializar SDL (`SDL_Init(SDL_INIT_VIDEO)`)
- Crear y poseer la ventana principal (`std::unique_ptr<Window>`)
- Ejecutar el bucle principal hasta `SDL_EVENT_QUIT`
- Propagar eventos a `Input::OnEvent()`
- Manejar el evento de resize (`SDL_EVENT_WINDOW_RESIZED` → `Window::UpdateSize`)
- Llamar a `Time::Update()` cada frame
- Llamar a `OnUpdate(deltaTime)` cada frame

**Flujo dentro de `Run()`:**
```
Time::Update()
↓
SDL_PollEvent loop:
    Input::OnEvent(event)
    if QUIT → m_IsRunning = false
    if RESIZE → m_Window->UpdateSize(w, h)
↓
OnUpdate(deltaTime)   ← actualmente vacío
↓
m_Window->OnUpdate()  ← clear + present directo en SDL
```

**Estado interno:**
| Campo | Tipo | Rol |
|---|---|---|
| `m_IsRunning` | `bool` | Controla el bucle principal |
| `m_SdlInitialized` | `bool` | Indica si hay que llamar `SDL_Quit()` en destructor |
| `m_Window` | `unique_ptr<Window>` | Ventana principal |

**Limitación crítica:** `OnUpdate(float)` es un método **privado** vacío.
El código del juego (`game/main.cpp`) no puede conectar lógica propia.
Actualmente la única forma de añadir lógica sería modificar el código del motor.

---

### 3.2 `Window` — Ventana SDL

**Archivos:** `engine/include/Platform/Window.h`, `engine/src/Platform/SDLWindow.cpp`

**Responsabilidades:**
- Crear `SDL_Window` con `SDL_WINDOW_RESIZABLE`
- Crear `SDL_Renderer` (SDL's hardware-accelerated 2D renderer)
- Activar VSync: `SDL_SetRenderVSync(renderer, 1)`
- Exponer el handle nativo para integrar otros renderers: `GetNativeWindow()`, `GetRenderer()`
- Limpiar la pantalla y presentar el frame en `OnUpdate()`
- Actualizar el viewport interno al redimensionar

**Diseño de headers:** `Window.h` usa forward declarations (`struct SDL_Window;`,
`struct SDL_Renderer;`) en lugar de incluir `<SDL3/SDL.h>`. Esto mantiene SDL
fuera de la API pública del motor: los consumidores del motor no necesitan
tener SDL en su include path.

**`OnUpdate()` actual:**
```cpp
SDL_SetRenderDrawColor(m_Renderer, 20, 20, 25, 255);  // color de fondo fijo
SDL_RenderClear(m_Renderer);
// [aquí irán futuras llamadas de render]
SDL_RenderPresent(m_Renderer);
```

**Limitación:** `OnUpdate()` mezcla clear + present. El color de fondo está
hardcodeado. No está conectado a `IRenderer2D`.

---

### 3.3 `Time` — Sistema de tiempo

**Archivos:** `engine/include/Core/Time.h`, `engine/src/Core/Time.cpp`

**Implementación:** usa `std::chrono::high_resolution_clock`. Variables estáticas
en el `.cpp` (no miembros de clase) guardan el tiempo de inicio y el del frame previo.

**API:**
| Método | Descripción |
|---|---|
| `Time::Init()` | Registra el instante de inicio. Llamado en `Application` constructor. |
| `Time::Update()` | Calcula el deltaTime desde la última llamada. Llamado al inicio de cada frame. |
| `Time::GetDeltaTime()` | Segundos entre el frame actual y el anterior. |
| `Time::GetTime()` | Segundos totales desde `Init()`. |
| `Time::GetFPS()` | `1.0f / deltaTime`. Retorna 0 si deltaTime ≈ 0. |

**Nota:** el deltaTime no tiene cap. Si el proceso se suspende 1 segundo,
el siguiente `GetDeltaTime()` retorna 1.0. Sin el cap, la física (cuando exista)
podría explotar. Ver sección 5.

---

### 3.4 `Input` — Estado de teclado y ratón

**Archivos:** `engine/include/Platform/Input.h`, `engine/src/Platform/Input.cpp`

**Implementación:** arrays estáticos actualizados por eventos SDL en `OnEvent()`.

**Estado almacenado:**
| Campo | Tipo | Contenido |
|---|---|---|
| `s_CurrentKeys`, `s_PrevKeys` | `array<bool, SDL_SCANCODE_COUNT>` | `true` si la tecla está pulsada, sea en este frame o en el anterior |
| `s_CurrentMouseButtons`, `s_PrevMouseButtons` | `array<bool, 5>` | `true` si el botón está pulsado, sea en este frame o en el anterior |
| `s_MouseX`, `s_MouseY` | `int` | Posición actual del cursor |

**Eventos manejados:**
- `SDL_EVENT_KEY_DOWN` / `SDL_EVENT_KEY_UP` → actualiza `s_CurrentKeys[scancode]`
- `SDL_EVENT_MOUSE_BUTTON_DOWN` / `SDL_EVENT_MOUSE_BUTTON_UP` → actualiza `s_CurrentMouseButtons[button]`
- `SDL_EVENT_MOUSE_MOTION` → actualiza `s_MouseX`, `s_MouseY`

~~**Limitación crítica:** solo almacena el estado del frame actual.
No hay `s_PrevKeys[]` → no es posible implementar `IsKeyJustPressed()` ni
`IsKeyJustReleased()`. Esto limita cualquier lógica de juego que necesite
reaccionar a un solo evento de pulsación (saltos, disparos, interacciones).~~

**Nota de botones de ratón en SDL3:** SDL usa `SDL_BUTTON_LEFT = 1`,
`SDL_BUTTON_MIDDLE = 2`, `SDL_BUTTON_RIGHT = 3`. El índice 0 del array nunca
se usa. Se usa un array de 6 para dejar espacio para los 2 botones laterales

---

### 3.5 `Log` — Sistema de logging

**Archivos:** `engine/include/Core/log.h`, `engine/src/Core/log.cpp`

**Niveles disponibles (en orden ascendente):**
`Trace → Info → Warn → Error → Critical`

**Comportamiento:**
- `Log::SetLevel(LogLevel::Warn)` silencia Trace e Info.
- `Error` y `Critical` van a `std::cerr`. El resto a `std::cout`.
- Formato: `[EntityEngine][LEVEL] mensaje\n`

**Macros:**
```cpp
EE_LOG_TRACE("msg")    // → Log::Trace("msg")
EE_LOG_INFO("msg")     // → Log::Info("msg")
EE_LOG_WARN("msg")     // → Log::Warn("msg")
EE_LOG_ERROR("msg")    // → Log::Error("msg")
EE_LOG_CRITICAL("msg") // → Log::Critical("msg")
```

**Limitación:** las macros no se eliminan en Release. En builds de distribución
debería añadirse `#ifdef NDEBUG` para que las macros Trace/Info sean no-op.

---

### 3.6 `IRenderer2D` — Interfaz de renderizado

**Archivo:** `engine/include/Render/IRenderer2D.h`

**Interfaz actual:**
```cpp
struct Color { uint8_t r, g, b, a; };

class IRenderer2D {
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void Clear(const Color& color) = 0;
};
```

**Limitación:** la interfaz no tiene métodos de dibujo de sprites, rectángulos,
líneas, ni texto. Es un esqueleto que solo cubre el ciclo de frame.

---

### 3.7 `SDLRenderer2D` — Implementación SDL de `IRenderer2D`

**Archivos:**
- Header en dos lugares (inconsistencia):
  - `engine/include/Render/Backend/SDLRenderer2D.h` → header público
  - `engine/src/Render/SDLRenderer2D.h` → parece un header de implementación (¿.cpp faltante?)
- Implementación actual en: `engine/src/Render/SDLRenderer2D.h` (el archivo .h contiene el código .cpp)

**Implementación:**
```cpp
SDLRenderer2D(SDL_Renderer* renderer);  // no posee el renderer, solo lo referencia
void BeginFrame() override;             // actualmente vacío
void EndFrame() override;               // → SDL_RenderPresent
void Clear(const Color& color) override;// → SDL_SetRenderDrawColor + SDL_RenderClear
```

**Limitación crítica:** `SDLRenderer2D` **no está conectado al bucle principal**.
`Application` no crea ni usa esta clase. El rendering actual va directamente
por `Window::OnUpdate()`. Esta clase existe pero no tiene efecto en la aplicación.

---

## 4. Flujo de datos completo por frame

```
Application::Run()
│
├─ Time::Update()
│   └─ Calcula s_DeltaTime = (now - s_LastFrameTime).count()
│
├─ SDL_PollEvent(&event) [loop]
│   ├─ Input::OnEvent(event)
│   │   ├─ KEY_DOWN → s_Keys[scancode] = true
│   │   ├─ KEY_UP   → s_Keys[scancode] = false
│   │   ├─ MOUSE_DOWN → s_MouseButtons[btn] = true
│   │   ├─ MOUSE_UP   → s_MouseButtons[btn] = false
│   │   └─ MOUSE_MOTION → s_MouseX, s_MouseY = event.motion.x/y
│   ├─ QUIT → m_IsRunning = false
│   └─ WINDOW_RESIZED → m_Window->UpdateSize(w, h)
│       └─ SDL_SetRenderViewport(renderer, {0,0,w,h})
│
├─ Application::OnUpdate(deltaTime)  ← VACÍO actualmente
│
└─ Window::OnUpdate()
    ├─ SDL_SetRenderDrawColor(renderer, 20, 20, 25, 255)
    ├─ SDL_RenderClear(renderer)
    └─ SDL_RenderPresent(renderer)
```

**SDLRenderer2D no aparece en este flujo.** Es una clase huérfana por ahora.

---

## 5. Limitaciones y deuda técnica conocida

| ID | Sistema | Problema | Impacto |
|---|---|---|---|
| L1 | `Application` | `OnUpdate()` es privado y vacío. El código del juego no puede conectar lógica. | Bloqueante para cualquier juego real. |
| L2 | ~~`Input`~~ | ~~Sin frame anterior → imposible `IsKeyJustPressed`/`IsKeyJustReleased`.~~ |~~Saltos, disparos, interacciones no funcionan correctamente.~~|
| L3 | `Window::OnUpdate()` | Mezcla clear + present. Color de fondo hardcodeado. No usa `IRenderer2D`. | El renderer abstracto es inútil hasta resolver esto. |
| L4 | `Time` | Sin cap en deltaTime. Un spike largo (>50ms) no se limita. | Futuro sistema de física puede explotar. |
| L5 | `Log` | Macros no se deshabilitan en Release (`NDEBUG`). | Overhead innecesario y logs en builds de distribución. |
| L6 | General | No hay sistema de entidades, recursos, cámara, física, audio, texto. | Motor incompleto para cualquier juego. |
| L7 | `game/main.cpp` | El include path usa ruta relativa `../../engine/include/...`. | Frágil. Debería usar las rutas públicas via `target_include_directories`. |

---

*Última actualización: 2026-05-06*
