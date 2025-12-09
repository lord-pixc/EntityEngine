# EntityEngine

EntityEngine es un prototipo ligero de motor 2D escrito en C++ que utiliza SDL3
para gestionar la ventana y la entrada. Incluye utilidades básicas de tiempo y
logging para facilitar el desarrollo de pequeños juegos o demos técnicas.

## Características
- **Bucle principal listo para usar** mediante `EntityEngine::Application`,
  responsable de inicializar SDL, crear la ventana y despachar eventos.
- **Gestión de entrada** con la clase estática `Input`, que expone el estado de
  teclado y ratón.
- **Utilidades de tiempo** (`Time`) para medir delta time, FPS y tiempo total
  desde el arranque de la aplicación.
- **Sistema de logging** sencillo con niveles de severidad configurables.
- **Renderizado 2D** a través de una interfaz `IRenderer2D` y una
  implementación de referencia basada en SDL.

## Estructura del proyecto
- `engine/` contiene el motor como biblioteca:
  - `include/` headers públicos organizados por módulo (`Core`, `Render`,
    `Platform`).
  - `src/` implementación de cada módulo.
- `game/` ejecutable de ejemplo que consume el motor.
- `external/` dependencias externas (si aplica).
- `CMakeLists.txt` archivos de configuración de CMake para compilar la
  biblioteca y el juego.

## Requisitos previos
- Compilador C++17 o superior.
- [CMake 3.16+](https://cmake.org/) para la configuración del proyecto.
- Cabeceras y librerías de desarrollo de [SDL3](https://github.com/libsdl-org/SDL)
  disponibles en el sistema.

## Cómo compilar
1. Crear el directorio de compilación:
   ```bash
   cmake -S . -B build
   ```
2. Generar los binarios:
   ```bash
   cmake --build build
   ```

Esto produce la biblioteca del motor y el ejecutable de ejemplo en `build/`.

## Cómo ejecutar
Después de compilar, ejecuta el ejemplo desde la raíz del proyecto:
```bash
./build/game/game
```
Si SDL no encuentra los backends de vídeo del sistema, revisa la instalación de
las dependencias (drivers, paquetes de desarrollo o variables de entorno como
`SDL_VIDEODRIVER`).

## Uso rápido del motor
- Incluye `engine/include/Core/Application.h` y crea una instancia de
  `EntityEngine::Application` con el título y dimensiones deseadas.
- Llama a `Run()` para entrar en el bucle principal.
- Consulta `Input::IsKeyPressed` o `Input::IsMouseButtonPressed` para reaccionar
  a la entrada del usuario, y `Input::GetMouseX`/`GetMouseY` para la posición
  del cursor.
- Utiliza `Time::GetDeltaTime()` para que tu lógica sea independiente de la
  velocidad de fotogramas y `Time::GetFPS()` para mostrar diagnósticos.
- Ajusta el nivel mínimo de logs con `Log::SetLevel` y emite mensajes usando las
  macros `EE_LOG_*`.

## Extensión y personalización
- La interfaz `IRenderer2D` permite añadir nuevos backends (por ejemplo, OpenGL
  o Vulkan) implementando `BeginFrame`, `Clear` y `EndFrame`.
- `Window` expone `GetNativeWindow()` y `GetRenderer()` para integrar otras
  APIs de renderizado o librerías de UI.
- `Application::OnUpdate` es el punto pensado para conectar sistemas de escena,
  entidades y lógica específica del juego.

## Contribuir
Las contribuciones son bienvenidas. Si encuentras problemas o tienes sugerencias
para nuevas características, abre un issue o envía un pull request describiendo
los cambios propuestos.
