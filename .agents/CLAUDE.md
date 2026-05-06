# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Run

```bash
# Configure (run once, or when CMakeLists.txt changes)
cmake -S . -B build

# Build everything
cmake --build build

# Run the demo game
./build/game/Game
```

SDL3 must be pre-built and available at `external/SDL3/build/` (the CMake config searches there via `SDL3_DIR`). If SDL can't find a video backend at runtime, check your drivers or set `SDL_VIDEODRIVER`.

The project uses **C++23** (`CMAKE_CXX_STANDARD 23`).

There are no tests yet.

## Architecture

The repo is split into two CMake targets:

- **`Engine`** — static library built from `engine/src/**`. Public headers live in `engine/include/` organized into three modules.
- **`Game`** — executable in `game/src/` that links against `Engine`.

### Module breakdown

| Module | Public header | Role |
|---|---|---|
| `Core/Application` | `Application.h` | Owns the main loop: initializes SDL, creates `Window`, drives `Time` and `Input`, calls `OnUpdate()` each frame. Entry point for all engine setup. |
| `Core/Time` | `Time.h` | Static utility. Call `Time::Init()` once, `Time::Update()` each frame. Provides `GetDeltaTime()`, `GetTime()`, `GetFPS()`. |
| `Core/Log` | `log.h` | Static logger writing to stdout/stderr with severity levels (`Trace`→`Critical`). Use the `EE_LOG_*` macros. Configure minimum level with `Log::SetLevel(LogLevel::Warn)`. |
| `Platform/Window` | `Window.h` | Wraps `SDL_Window` + `SDL_Renderer`. SDL headers are not exposed in the public interface (forward declarations only). Exposes `GetNativeWindow()` and `GetRenderer()` for integrating other rendering APIs. |
| `Platform/Input` | `Input.h` | Static input state. `Application::Run()` feeds SDL events through `Input::OnEvent()`. Query with `IsKeyPressed(SDL_Scancode)`, `IsMouseButtonPressed(uint8_t)`, `GetMouseX/Y()`. |
| `Render/IRenderer2D` | `IRenderer2D.h` | Abstract interface: `BeginFrame()`, `EndFrame()`, `Clear(Color)`. Add new backends (OpenGL, Vulkan) by implementing this. |
| `Render/SDLRenderer2D` | `Render/Backend/SDLRenderer2D.h` | Concrete `IRenderer2D` wrapping an `SDL_Renderer*` it does not own — lifetime is managed by `Window`. |

### Key design decisions

- **`Application::OnUpdate(float deltaTime)`** is the extension point for game logic (scenes, entities, etc.). It is currently empty and called every frame after input processing.
- `Window::OnUpdate()` handles the SDL render present call directly today; the `SDLRenderer2D` wrapper exists but is not yet wired into the main loop — the intent is to route rendering through `IRenderer2D` implementations.
- SDL headers are kept out of public engine headers via forward declarations, so game code does not need to include SDL directly.
- VSync is enabled by default (`SDL_SetRenderVSync(renderer, 1)`).
