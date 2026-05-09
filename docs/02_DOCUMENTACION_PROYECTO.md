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

#### Stack tecnológico

- Lenguaje: C++23
- Sistema de ventana/render: SDL3
- Build system: CMake 3.23+
- SDL3 pre-compilado en `external/SDL3/build/`

#### Dos targets CMake

- `Engine` → `build/engine/libEngine.a` (biblioteca estática)
- `Game` → `build/game/Game` (ejecutable de demo)

---

## 2. Estructura de archivos

```txt
EntityEngine/
├── engine/
│   ├── include/           ← API pública del motor (sin headers SDL en la interfaz)
│   │   ├── Core/
│   │   │   ├── Application.h   ← Punto de entrada del motor
│   │   │   ├── Time.h          ← Utilidades de tiempo
│   │   │   └── log.h           ← Sistema de logging
│   │   ├── Platform/
│   │   │   ├── Window.h        ← Abstracción de ventana (SDL ocultado)
│   │   │   ├── InputCodes.h    ← Códigos propios del motor para teclas/ratón
│   │   │   ├── Input.h         ← Estado de teclado y ratón
│   │   │   └── InputMap.h      ← Acciones configurables y mappings
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
│       │   ├── InputMap.cpp
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

Archivos: `engine/include/Core/Application.h`, `engine/src/Core/Application.cpp`

#### Responsabilidades

- Inicializar SDL (`SDL_Init(SDL_INIT_VIDEO)`)
- Crear y poseer la ventana principal (`std::unique_ptr<Window>`)
- Ejecutar el bucle principal hasta `SDL_EVENT_QUIT`
- Propagar eventos SDL al sistema `Input` mediante una función interna privada
- Manejar el evento de resize (`SDL_EVENT_WINDOW_RESIZED` → `Window::UpdateSize`)
- Llamar a `Time::Update()` cada frame
- Llamar a `OnUpdate(deltaTime)` cada frame

#### Flujo dentro de `Run()`

```txt
Time::Update()
↓
SDL_PollEvent loop:
    Input::OnEvent(&event)  ← privado; solo Application lo llama
    if QUIT → m_IsRunning = false
    if RESIZE → m_Window->UpdateSize(w, h)
↓
OnUpdate(deltaTime)   ← actualmente vacío
↓
m_Window->OnUpdate()  ← clear + present directo en SDL
↓
Input::EndFrame()     ← copia estado actual a estado previo
```

#### Estado interno

| Campo              | Tipo                 | Rol                                                 |
| ------------------ | -------------------- | --------------------------------------------------- |
| `m_IsRunning`      | `bool`               | Controla el bucle principal                         |
| `m_SdlInitialized` | `bool`               | Indica si hay que llamar `SDL_Quit()` en destructor |
| `m_Window`         | `unique_ptr<Window>` | Ventana principal                                   |

Limitación crítica: `OnUpdate(float)` es un método **privado** vacío.
El código del juego (`game/main.cpp`) no puede conectar lógica propia.
Actualmente la única forma de añadir lógica sería modificar el código del motor.

---

### 3.2 `Window` — Ventana SDL

Archivos: `engine/include/Platform/Window.h`, `engine/src/Platform/SDLWindow.cpp`

#### Responsabilidades

- Crear `SDL_Window` con `SDL_WINDOW_RESIZABLE`
- Crear `SDL_Renderer` (SDL's hardware-accelerated 2D renderer)
- Activar VSync: `SDL_SetRenderVSync(renderer, 1)`
- Exponer el handle nativo para integrar otros renderers: `GetNativeWindow()`, `GetRenderer()`
- Limpiar la pantalla y presentar el frame en `OnUpdate()`
- Actualizar el viewport interno al redimensionar

Diseño de headers: `Window.h` usa forward declarations (`struct SDL_Window;`,
`struct SDL_Renderer;`) en lugar de incluir `<SDL3/SDL.h>`. Esto mantiene SDL
fuera de la API pública del motor: los consumidores del motor no necesitan
tener SDL en su include path.

#### `OnUpdate()` actual

```cpp
SDL_SetRenderDrawColor(m_Renderer, 20, 20, 25, 255);  // color de fondo fijo
SDL_RenderClear(m_Renderer);
// [aquí irán futuras llamadas de render]
SDL_RenderPresent(m_Renderer);
```

Limitación: `OnUpdate()` mezcla clear + present. El color de fondo está
hardcodeado. No está conectado a `IRenderer2D`.

---

### 3.3 `Time` — Sistema de tiempo

Archivos: `engine/include/Core/Time.h`, `engine/src/Core/Time.cpp`

Implementación: usa `std::chrono::high_resolution_clock`. Variables estáticas
en el `.cpp` (no miembros de clase) guardan el tiempo de inicio y el del frame previo.

#### API

| Método                 | Descripción                                                                    |
| ---------------------- | ------------------------------------------------------------------------------ |
| `Time::Init()`         | Registra el instante de inicio. Llamado en `Application` constructor.          |
| `Time::Update()`       | Calcula el deltaTime desde la última llamada. Llamado al inicio de cada frame. |
| `Time::GetDeltaTime()` | Segundos entre el frame actual y el anterior.                                  |
| `Time::GetTime()`      | Segundos totales desde `Init()`.                                               |
| `Time::GetFPS()`       | `1.0f / deltaTime`. Retorna 0 si deltaTime ≈ 0.                                |

Nota: el deltaTime no tiene cap. Si el proceso se suspende 1 segundo,
el siguiente `GetDeltaTime()` retorna 1.0. Sin el cap, la física (cuando exista)
podría explotar. Ver sección 5.

---

### 3.4 `InputCodes` — Códigos públicos de input

Archivo: `engine/include/Platform/InputCodes.h`

Define tipos propios del motor para evitar que el código de juego dependa de
SDL:

| Tipo          | Contenido                                                                                                |
| ------------- | -------------------------------------------------------------------------------------------------------- |
| `KeyCode`     | Enum de teclas físicas del teclado. Sus valores coinciden con los scancodes de SDL3 usados internamente. |
| `MouseButton` | Enum de botones de ratón: `Left`, `Middle`, `Right`, `X1`, `X2`.                                         |

`KeyCode` está alineado con `SDL_SCANCODE_*` para que `Input.cpp` pueda indexar
arrays internos sin tablas de conversión complejas. `Input.cpp` contiene
`static_assert` que validan esa equivalencia contra la versión de SDL3 vendorizada
en `external/SDL3/`.

Regla importante: `InputCodes.h` no incluye headers de SDL. Es parte de la
API pública del motor.

---

### 3.5 `Input` — Estado de teclado y ratón

Archivos: `engine/include/Platform/Input.h`, `engine/src/Platform/Input.cpp`

Implementación: arrays estáticos actualizados por eventos SDL en una función
privada llamada por `Application`.

#### Estado almacenado

| Campo                                  | Tipo                          | Contenido                                               |
| -------------------------------------- | ----------------------------- | ------------------------------------------------------- |
| `s_CurrentKeys`, `s_PrevKeys`          | `array<bool, KeyCode::Count>` | Estado actual y anterior de cada tecla física           |
| `s_MouseButtons`, `s_PrevMouseButtons` | `array<bool, 6>`              | Estado actual y anterior de botones de ratón SDL `1..5` |
| `s_MouseX`, `s_MouseY`                 | `int`                         | Posición actual del cursor                              |

#### Eventos manejados

- `SDL_EVENT_KEY_DOWN` / `SDL_EVENT_KEY_UP` → actualiza `s_CurrentKeys[scancode]`
- `SDL_EVENT_MOUSE_BUTTON_DOWN` / `SDL_EVENT_MOUSE_BUTTON_UP` → actualiza `s_MouseButtons[button]`
- `SDL_EVENT_MOUSE_MOTION` → actualiza `s_MouseX`, `s_MouseY`

#### API pública

```cpp
Input::IsKeyHeld(KeyCode::Space);
Input::IsKeyJustPressed(KeyCode::Escape);
Input::IsKeyJustReleased(KeyCode::A);

Input::IsMouseButtonHeld(MouseButton::Left);
Input::IsMouseButtonJustPressed(MouseButton::X1);
Input::IsMouseButtonJustReleased(MouseButton::Right);
```

`Input.h` ya no expone `SDL_Event` ni `SDL_Scancode`. SDL queda encapsulado en
`Input.cpp` y `Application.cpp`.

Nota de botones de ratón en SDL3: SDL usa `SDL_BUTTON_LEFT = 1`,
`SDL_BUTTON_MIDDLE = 2`, `SDL_BUTTON_RIGHT = 3`, `SDL_BUTTON_X1 = 4`,
`SDL_BUTTON_X2 = 5`. El índice 0 del array nunca se usa. Se usa un array de 6
para poder indexar directamente `1..5` y cubrir los dos botones laterales de
muchos ratones gaming.

---

### 3.6 `InputMap` — Acciones y mappings

Archivos: `engine/include/Platform/InputMap.h`, `engine/src/Platform/InputMap.cpp`

`InputMap` es una capa semántica encima de `Input`. Permite que el juego consulte
acciones de gameplay en lugar de teclas físicas:

```cpp
inputMap.Bind("Jump", KeyCode::Space);
inputMap.Bind("Attack", MouseButton::Left);

if (inputMap.IsActionJustPressed("Jump"))
{
    // saltar
}
```

#### Conceptos

| Concepto | Descripción                                                                       |
| -------- | --------------------------------------------------------------------------------- |
| Action   | Nombre libre definido por el juego: `"Jump"`, `"Attack"`, `"Pause"`, etc.         |
| Binding  | Entrada física que activa una action: `KeyCode::Space`, `MouseButton::Left`, etc. |
| Mapping  | Tabla `action -> lista de bindings`.                                              |

#### Representación actual

```cpp
struct KeyBinding { KeyCode Key; };
struct MouseButtonBinding { MouseButton Button; };
using InputBinding = std::variant<KeyBinding, MouseButtonBinding>;

std::unordered_map<std::string, std::vector<InputBinding>> m_Actions;
```

Esto permite múltiples bindings por acción:

```txt
"Jump"   -> [KeyCode::Space, KeyCode::W]
"Attack" -> [MouseButton::Left, KeyCode::LeftCtrl]
```

#### API actual

| Método                             | Descripción                                              |
| ---------------------------------- | -------------------------------------------------------- |
| `Bind(action, KeyCode)`            | Agrega una tecla a la action.                            |
| `Bind(action, MouseButton)`        | Agrega un botón de ratón a la action.                    |
| `HasAction(action)`                | Indica si la action existe en el mapa.                   |
| `ClearAction(action)`              | Elimina todos los bindings de una action.                |
| `ReplaceBinding(action, old, new)` | Reemplaza un binding concreto si existe.                 |
| `IsActionHeld(action)`             | `true` si cualquier binding de la action está mantenido. |
| `IsActionJustPressed(action)`      | `true` si cualquier binding se presionó este frame.      |
| `IsActionJustReleased(action)`     | `true` si cualquier binding se soltó este frame.         |

Limitación: todavía no está integrado en una demo ni existe un punto de
extensión de juego donde usarlo sin modificar el motor. Ver deuda técnica.

---

### 3.7 `Log` — Sistema de logging

Archivos: `engine/include/Core/log.h`, `engine/src/Core/log.cpp`

#### Niveles disponibles (en orden ascendente)

`Trace → Info → Warn → Error → Critical`

#### Comportamiento

- `Log::SetLevel(LogLevel::Warn)` silencia Trace e Info.
- `Error` y `Critical` van a `std::cerr`. El resto a `std::cout`.
- Formato: `[EntityEngine][LEVEL] mensaje\n`

#### Macros

```cpp
EE_LOG_TRACE("msg")    // → Log::Trace("msg")
EE_LOG_INFO("msg")     // → Log::Info("msg")
EE_LOG_WARN("msg")     // → Log::Warn("msg")
EE_LOG_ERROR("msg")    // → Log::Error("msg")
EE_LOG_CRITICAL("msg") // → Log::Critical("msg")
```

Limitación: las macros no se eliminan en Release. En builds de distribución
debería añadirse `#ifdef NDEBUG` para que las macros Trace/Info sean no-op.

---

### 3.8 `IRenderer2D` — Interfaz de renderizado

Archivo: `engine/include/Render/IRenderer2D.h`

#### Interfaz actual

```cpp
struct Color { uint8_t r, g, b, a; };

class IRenderer2D {
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void Clear(const Color& color) = 0;
};
```

Limitación: la interfaz no tiene métodos de dibujo de sprites, rectángulos,
líneas, ni texto. Es un esqueleto que solo cubre el ciclo de frame.

---

### 3.9 `SDLRenderer2D` — Implementación SDL de `IRenderer2D`

#### Archivos

- Header en dos lugares (inconsistencia):
  - `engine/include/Render/Backend/SDLRenderer2D.h` → header público
  - `engine/src/Render/SDLRenderer2D.h` → parece un header de implementación (¿.cpp faltante?)
- Implementación actual en: `engine/src/Render/SDLRenderer2D.h` (el archivo .h contiene el código .cpp)

#### Implementación

```cpp
SDLRenderer2D(SDL_Renderer* renderer);  // no posee el renderer, solo lo referencia
void BeginFrame() override;             // actualmente vacío
void EndFrame() override;               // → SDL_RenderPresent
void Clear(const Color& color) override;// → SDL_SetRenderDrawColor + SDL_RenderClear
```

Limitación crítica: `SDLRenderer2D` **no está conectado al bucle principal**.
`Application` no crea ni usa esta clase. El rendering actual va directamente
por `Window::OnUpdate()`. Esta clase existe pero no tiene efecto en la aplicación.

---

## 4. Flujo de datos completo por frame

```txt
Application::Run()
│
├─ Time::Update()
│   └─ Calcula s_DeltaTime = (now - s_LastFrameTime).count()
│
├─ SDL_PollEvent(&event) [loop]
│   ├─ Input::OnEvent(&event) [privado]
│   │   ├─ KEY_DOWN → s_CurrentKeys[scancode] = true
│   │   ├─ KEY_UP   → s_CurrentKeys[scancode] = false
│   │   ├─ MOUSE_DOWN → s_MouseButtons[btn] = true
│   │   ├─ MOUSE_UP   → s_MouseButtons[btn] = false
│   │   └─ MOUSE_MOTION → s_MouseX, s_MouseY = event.motion.x/y
│   ├─ QUIT → m_IsRunning = false
│   └─ WINDOW_RESIZED → m_Window->UpdateSize(w, h)
│       └─ SDL_SetRenderViewport(renderer, {0,0,w,h})
│
├─ Application::OnUpdate(deltaTime)  ← VACÍO actualmente
│
├─ Window::OnUpdate()
│   ├─ SDL_SetRenderDrawColor(renderer, 20, 20, 25, 255)
│   ├─ SDL_RenderClear(renderer)
│   └─ SDL_RenderPresent(renderer)
│
└─ Input::EndFrame()
    └─ s_PrevKeys = s_CurrentKeys; s_PrevMouseButtons = s_MouseButtons
```

Consulta de actions: `InputMap` no participa automáticamente del bucle.
El juego debe poseer un `InputMap` y consultarlo durante su lógica:

```cpp
if (inputMap.IsActionJustPressed("Jump"))
{
    // gameplay
}
```

```txt
InputMap::IsActionJustPressed("Jump")
└─ busca bindings de "Jump"
   ├─ KeyBinding → Input::IsKeyJustPressed(key)
   └─ MouseButtonBinding → Input::IsMouseButtonJustPressed(button)
```

SDLRenderer2D no aparece en este flujo. Es una clase huérfana por ahora.

---

## 5. Limitaciones y deuda técnica conocida

| ID  | Sistema              | Problema                                                                                                                        | Impacto                                                                                             |
| --- | -------------------- | ------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------- |
| L1  | `Application`        | `OnUpdate()` es privado y vacío. El código del juego no puede conectar lógica.                                                  | Bloqueante para cualquier juego real.                                                               |
| L2  | ~~`Input`~~          | ~~Sin frame anterior → imposible `IsKeyJustPressed`/`IsKeyJustReleased`.~~                                                      | ~~Saltos, disparos, interacciones no funcionan correctamente.~~                                     |
| L3  | `Window::OnUpdate()` | Mezcla clear + present. Color de fondo hardcodeado. No usa `IRenderer2D`.                                                       | El renderer abstracto es inútil hasta resolver esto.                                                |
| L4  | ~~`Time`~~           | ~~Sin cap en deltaTime. Un spike largo (>50ms) no se limita.~~                                                                  | Futuro sistema de física puede explotar.                                                            |
| L5  | `Log`                | Macros no se deshabilitan en Release (`NDEBUG`).                                                                                | Overhead innecesario y logs en builds de distribución.                                              |
| L6  | General              | No hay sistema de entidades, recursos, cámara, física, audio, texto.                                                            | Motor incompleto para cualquier juego.                                                              |
| L7  | `game/main.cpp`      | El include path usa ruta relativa `../../engine/include/...`.                                                                   | Frágil. Debería usar las rutas públicas via `target_include_directories`.                           |
| L8  | `InputMap`           | API inicial sin helpers cómodos para construir/reemplazar bindings. `ReplaceBinding` requiere crear `InputBinding` manualmente. | El remapeo funciona, pero es incómodo para el game dev.                                             |
| L9  | `InputMap`           | `Bind()` permite duplicar el mismo binding varias veces en una action.                                                          | Estado redundante y posibles confusiones al serializar o mostrar controles.                         |
| L10 | `InputMap`           | No hay ejes (`MoveX`, `MoveY`), combinaciones (`Ctrl+S`), contextos (`Gameplay/Menu`) ni persistencia en archivo.               | Sistema suficiente para acciones binarias simples, incompleto para controles configurables maduros. |
| L11 | `InputMap`           | No está demostrado/integrado en `game/main.cpp` porque `Application` aún no expone lógica de juego.                             | El sistema existe, pero no hay ejemplo real de uso en gameplay.                                     |

---

*Última actualización: 2026-05-07*
