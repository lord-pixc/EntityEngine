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
Estado del mundo (posiciones, texturas, cámara)
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
- Enviar la imagen terminada al monitor.

El sistema de renderizado **no decide qué dibujar**. Eso lo decide el juego.
El sistema solo provee las herramientas para hacerlo.

### Por qué no se dibuja directamente con SDL

SDL3 tiene funciones que dibujan directamente sobre el renderer. En un proyecto
pequeño está bien usarlas directo.

En un motor, el problema es que si el código del juego llama a funciones SDL
directamente, el motor queda atado a SDL para siempre. Cambiar a OpenGL o
Vulkan requeriría reescribir todo el código del juego.

La solución es una **interfaz abstracta**: el juego llama a métodos del motor
(`DrawSprite`, `DrawRect`). El motor decide internamente cómo traducir eso a
SDL, OpenGL, Vulkan, etc.

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

Toda la arquitectura de abstracción está preparada pero desconectada.

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

### En pseudocódigo

Sin abstracción, el juego hace cosas específicas de SDL:

```txt
llamar función de SDL con color rojo
llamar función de SDL para dibujar rectángulo en posición X
```

Con abstracción, el juego hace esto:

```txt
renderer.DrawRect(posición, color)
```

La implementación concreta de `DrawRect` existe en la clase que implementa
`IRenderer2D`. Esa clase es la única que sabe cómo hablarle a SDL, OpenGL, etc.

### Diseño actual de IRenderer2D

La interfaz hoy solo cubre el ciclo básico de frame:

```txt
IRenderer2D:
  + BeginFrame()  — preparar el renderer
  + EndFrame()    — presentar la imagen al monitor
  + Clear(color)  — limpiar la pantalla con ese color
```

Faltan todos los métodos de dibujo real: sprites, rectángulos, líneas.
Eso se añade en las siguientes fases.

---

## 4. El ciclo de un frame de renderizado

Cada frame, el sistema sigue esta secuencia fija:

```txt
1. BeginFrame    ← preparar el renderer para recibir órdenes de dibujo
2. Clear(color)  ← limpiar la pantalla con el color de fondo
3. DrawXxx(...)  ← todas las órdenes de dibujo del frame
4. EndFrame      ← enviar la imagen terminada al monitor
```

Esto es análogo a pintar un cuadro:

- `BeginFrame`: preparar el lienzo.
- `Clear`: pintar el lienzo de un color base.
- `DrawXxx`: añadir todos los elementos encima.
- `EndFrame`: mostrar el cuadro terminado.

### Por qué `BeginFrame` y `EndFrame` son importantes

En SDL3 el renderer es una máquina de estados. En OpenGL hay que abrir y
cerrar contextos. En Vulkan hay command buffers y render passes con sus
propios ciclos de apertura y cierre.

`BeginFrame` y `EndFrame` son los puntos de entrada y salida que garantizan
que cada backend puede manejar su propia inicialización y finalización de frame
sin que el código del juego sepa cómo funciona internamente.

---

## 5. Fase 0.3 — Conectar IRenderer2D al bucle

Este es el primer paso real a implementar. Desbloquea todo lo demás.

### El problema actual

`Window::OnUpdate()` mezcla dos responsabilidades:

```txt
Window::OnUpdate():
  limpiar la pantalla con color fijo
  [aquí irían draw calls]
  presentar el frame
```

`Application` llama eso cada frame. `SDLRenderer2D` nunca se invoca.

### Lo que hay que resolver

**Pregunta guía**: ¿quién debe poseer el renderer y quién debe llamarlo?

El renderer debe vivir en `Application`, no en `Window`. La ventana es
responsable de existir como superficie de dibujo. El renderer es responsable
de dibujar sobre esa superficie.

El flujo correcto es:

```txt
Application posee:
  - Window (la ventana del SO)
  - IRenderer2D (el objeto que dibuja sobre esa ventana)

Application::Run() cada frame:
  - gestionar eventos
  - llamar al juego (OnUpdate)
  - pedir al renderer que limpie, reciba draw calls, y presente
```

**Qué debe dejar de hacer `Window`:**

`Window::OnUpdate()` actualmente hace el clear y el present. Eso debe
desaparecer de `Window` y pasar a ser responsabilidad del renderer.

`Window` debe quedarse solo con lo que es responsabilidad de una ventana:
gestionar el handle nativo, el tamaño, el título.

**Cómo `Application` obtiene el renderer:**

`SDLRenderer2D` necesita el `SDL_Renderer*` que crea `Window`. `Window` ya
expone `GetRenderer()` exactamente para ese propósito. `Application` puede
crear el `SDLRenderer2D` pasándole ese puntero.

Hay que mirar cómo funciona `SDLRenderer2D`: no posee el renderer SDL,
solo lo usa. La propiedad del renderer SDL sigue siendo de `Window`.

### Qué se consigue con este paso

- El bucle principal pasa por `IRenderer2D` en vez de por `Window`.
- Añadir un backend OpenGL en el futuro solo requiere crear otro objeto
  que implemente `IRenderer2D` y pasarlo.
- `Window` queda con una sola responsabilidad.

---

## 6. Fase 1 — Primitivas y formas básicas

Antes de cargar imágenes, las formas simples son útiles para debug y prototipos.
Con un rectángulo puedes representar un personaje. Con una línea puedes ver
rayos de colisión. Sin necesitar ninguna imagen.

### Qué tipos nuevos hacen falta

La interfaz necesita describir posición y tamaño de forma agnóstica al backend.
Piensa qué estructura mínima representa "un rectángulo en el espacio 2D":

```txt
Rect:
  posición x, y
  tamaño w, h
```

Este tipo va en la interfaz pública, no en el backend.

### Qué métodos añadir a IRenderer2D

```txt
DrawRect(rect, color, filled)
  — filled=true: rectángulo relleno
  — filled=false: solo el borde

DrawLine(x0, y0, x1, y1, color)
  — segmento entre dos puntos

DrawCircle(cx, cy, radius, color)
  — SDL no tiene función nativa para círculos
  — se aproxima dibujando N segmentos de línea alrededor del centro
```

### Cómo implementar el círculo

SDL3 no tiene `DrawCircle`. La solución estándar es aproximarlo con líneas:
dividir la circunferencia en N ángulos (por ejemplo 32), calcular los puntos
en cada ángulo con trigonometría básica, y dibujar segmentos entre puntos
consecutivos.

Fórmula de punto en circunferencia para ángulo `a` y radio `r`:

```txt
punto.x = centro.x + cos(a) * r
punto.y = centro.y + sin(a) * r
```

Con eso tienes todo lo necesario para implementarlo.

---

## 7. Fase 2 — Texturas y sprites

Esta es la fase principal del renderizado visible.

### Qué es un sprite

Un sprite es una imagen 2D dibujada en una posición del mundo.

En la mayoría de motores, para dibujar un sprite necesitas:

- La imagen (textura) que quieres dibujar.
- `srcRect`: qué región de esa textura usar. Útil para sprite sheets donde
  múltiples personajes o frames están en una sola imagen grande.
- `dstRect`: dónde y con qué tamaño dibujarlo en pantalla.

```txt
Textura completa (512x512 píxeles)
┌─────────────────────────────────────┐
│                                     │
│  ┌──────┐  ← srcRect (32x32 px)     │
│  │ Img  │    la región que usamos   │
│  └──────┘                           │
└─────────────────────────────────────┘
                ↓ se dibuja en

┌──────────────────────┐
│ Pantalla             │
│   ┌──────────────┐   │
│   │   dstRect    │   │  puede tener distinto tamaño que el original
│   └──────────────┘   │
└──────────────────────┘
```

### Qué método añadir a IRenderer2D

```txt
DrawSprite(textura, srcRect, dstRect, rotation, tint)
  — textura: referencia a la imagen
  — srcRect: región fuente de la textura
  — dstRect: posición y tamaño en pantalla
  — rotation: grados de rotación (opcional, default 0)
  — tint: color multiplicativo (opcional, default blanco = sin tinte)
```

El tinte blanco `(255, 255, 255, 255)` significa "dibujar la imagen sin
modificar su color". Un tinte rojo haría que la imagen se vea rojiza.

### SDL3 y el tipo para la textura

SDL3 tiene `SDL_RenderTextureRotated` para dibujar con rotación. Busca en la
documentación de SDL3 qué parámetros espera para entender qué tipos necesitas
adaptar desde tu `Rect` y tu `Color`.

---

## 8. Fase 3 — Handles opacos

Este concepto es una de las decisiones de arquitectura más importantes del motor
y merece entenderse bien antes de implementar texturas.

### El problema: ¿cómo referencia el juego a una textura?

Opción A: puntero directo al objeto de la GPU.

```txt
// El juego guarda un SDL_Texture*
SDLTexture* miTextura = ...
renderer.DrawSprite(miTextura, ...)
```

Problema: la interfaz `IRenderer2D` quedaría con tipos SDL en su firma. Un
`OpenGLRenderer2D` no tiene `SDL_Texture`. La abstracción se rompe.

Opción B: una clase `Texture` propia del motor.

```txt
class Texture {
    SDL_Texture* m_SdlTexture;  // ← SDL se filtra al header público
    GLuint m_GlTexture;         // ← y lo mismo con OpenGL
    int width, height;
};
```

Problema: para soportar múltiples backends, `Texture` se llenaría de miembros
de cada backend. Sigue siendo una filtración.

Opción C: handle opaco (la solución correcta).

```txt
using TextureHandle = uint32_t;  // solo un número entero
```

El juego solo guarda ese número. El renderer mantiene internamente una tabla
que traduce ese número al objeto real del backend:

```txt
SDLRenderer2D internamente:
  tabla[1] → SDL_Texture* A
  tabla[2] → SDL_Texture* B

OpenGLRenderer2D internamente:
  tabla[1] → GLuint A
  tabla[2] → GLuint B
```

El juego llama a `DrawSprite(handle=1, ...)` sin saber qué hay dentro.

### Qué métodos implica esto en IRenderer2D

```txt
CreateTexture(ancho, alto, datos_de_pixeles) → TextureHandle
  — sube los píxeles a GPU y devuelve un handle

DestroyTexture(handle)
  — libera la textura de GPU

DrawSprite(handle, srcRect, dstRect, ...)
  — dibuja la textura identificada por ese handle
```

### La regla clave

Ningún tipo en `engine/include/Render/IRenderer2D.h` puede venir de SDL,
OpenGL o Vulkan. Solo tipos primitivos (`uint32_t`, `float`) y tipos propios
del motor (`Color`, `Rect`, `TextureHandle`).

---

## 9. Fase 4 — ResourceManager

El `ResourceManager` es el sistema que carga activos desde disco y los
entrega al juego como handles.

### Por qué es necesario

Sin `ResourceManager`, el juego tendría que:

1. Leer el archivo PNG desde disco.
2. Decodificarlo a píxeles RGBA.
3. Llamar a `CreateTexture` en el renderer.
4. Recordar destruir esa textura manualmente al terminar.

Además, si dos sistemas cargan la misma imagen independientemente, se subirían
dos copias idénticas a la GPU. Desperdicio.

### Qué hace

```txt
ResourceManager::Load("jugador.png") → TextureHandle
  — si ya estaba cargada: devuelve el handle existente (caché)
  — si no: lee el archivo, decodifica, sube a GPU, guarda en caché

ResourceManager::Unload("jugador.png")
  — libera la textura de GPU y borra de caché
```

La caché es simplemente una tabla `nombre_de_archivo → handle`.

### Cómo cargar imágenes PNG

SDL3 puede cargar imágenes con `SDL_LoadBMP` para BMP. Para PNG y JPG la
opción recomendada en el plan de acción a corto plazo es `SDL_image`.

A largo plazo, `stb_image` es una librería header-only (un solo `.h`) que
carga PNG, JPG, BMP y otros formatos sin depender de SDL. Buscala en GitHub:
`nothings/stb`. El archivo relevante es `stb_image.h`.

El flujo de carga con cualquier librería es:

```txt
leer archivo del disco
obtener: ancho, alto, puntero a datos RGBA
llamar a renderer.CreateTexture(ancho, alto, datos)
liberar la memoria de los datos (ya están en GPU)
guardar el handle en la caché
```

---

## 10. Fase 5 — Cámara 2D

La cámara define qué parte del mundo es visible en pantalla.

### El problema sin cámara

Sin cámara, las coordenadas del juego son directamente píxeles de pantalla.
Si el jugador está en la posición `(500, 300)` del mundo, se dibuja en el
píxel `(500, 300)` de la ventana.

Esto funciona si el nivel cabe en pantalla. En cuanto el nivel es más grande,
el jugador desaparece fuera del borde.

### Qué es una cámara 2D

La cámara define un punto del mundo que aparece en el centro de la pantalla,
y un zoom que determina cuánto mundo es visible:

```txt
Camera2D:
  posición x, y  — punto del mundo centrado en pantalla
  zoom           — 1.0=normal, 2.0=más cerca, 0.5=más lejos
```

### La transformación mundo → pantalla

Para saber dónde dibujar un objeto del mundo en pantalla, la fórmula es:

```txt
screenX = (worldX - camera.x) * zoom + (ancho_pantalla / 2)
screenY = (worldY - camera.y) * zoom + (alto_pantalla / 2)
```

Ejemplo con cámara en `(200, 150)`, zoom `1.0`, pantalla `800x600`:

```txt
Objeto en el mundo en (220, 160):
  screenX = (220 - 200) * 1.0 + 400 = 420
  screenY = (160 - 150) * 1.0 + 300 = 310
```

Con zoom `2.0`, el mismo objeto:

```txt
  screenX = (220 - 200) * 2.0 + 400 = 440
  screenY = (160 - 150) * 2.0 + 300 = 320
```

Con más zoom el objeto se aleja más del centro, porque el mundo se ve
"más grande" relativamente.

### La transformación inversa (pantalla → mundo)

Necesaria para saber en qué punto del mundo hizo click el ratón:

```txt
worldX = (screenX - ancho_pantalla / 2) / zoom + camera.x
worldY = (screenY - alto_pantalla / 2) / zoom + camera.y
```

### Mundo vs espacio de pantalla (UI)

La cámara afecta todo lo que se dibuja. Pero la UI (barra de salud, puntuación,
menús) debe dibujarse en posiciones fijas de pantalla, no del mundo.

El motor necesita un modo "espacio de pantalla" donde la transformación de
cámara se ignora:

```txt
// Dibujar el mundo (afectado por la cámara)
renderer.SetCamera(gameCamera)
renderer.DrawSprite(jugador, ...)
renderer.DrawSprite(enemigo, ...)

// Dibujar la UI (siempre en la misma posición de pantalla)
renderer.BeginScreenSpace()
renderer.DrawSprite(barraSalud, posicionFija)
renderer.EndScreenSpace()
```

### Cómo implementarlo en SDL3

SDL3 tiene funciones que permiten aplicar escala y offset global al renderer.
Busca `SDL_SetRenderScale` y cómo afecta a todas las llamadas de dibujo
posteriores. Eso es el mecanismo que implementa el zoom de la cámara.

El offset de la cámara (desplazamiento del mundo) puede aplicarse de varias
formas: modificando el viewport o ajustando manualmente las coordenadas antes
de cada draw call. La primera opción es más limpia.

---

## 11. Reglas que debe respetar el sistema

### Regla 1: Ningún header público puede incluir SDL

Todo lo que está en `engine/include/` debe ser legible sin que el proyecto que
lo usa tenga SDL en su include path.

Los `.cpp` del motor sí pueden incluir SDL. Los `.h` públicos no.

Esta regla ya se cumple en `Window.h` (usa forward declaration `struct SDL_Window;`).
Debe respetarse en todo lo nuevo que se añada.

### Regla 2: Los tipos de la interfaz son agnósticos al backend

`Color`, `Rect`, `TextureHandle` son tipos propios del motor. No son tipos SDL,
OpenGL ni Vulkan. Si la firma de un método en `IRenderer2D.h` menciona
`SDL_Texture` o `SDL_FRect`, algo está mal.

### Regla 3: El renderer no posee la ventana

`SDLRenderer2D` recibe el `SDL_Renderer*` externo pero no es dueño de él.
No debe destruirlo. La ventana lo crea y lo destruye.

Esto ya está correcto en el código actual. Hay que mantenerlo al ampliar.

### Regla 4: La vida de las texturas la gestiona el ResourceManager

Una textura creada con `CreateTexture` debe ser destruida con `DestroyTexture`
antes de que el renderer se destruya. El `ResourceManager` es quien sabe qué
texturas están vivas. El juego no destruye texturas directamente.

---

## 12. Orden de implementación recomendado

### Paso 1 — Conectar IRenderer2D (Fase 0.3 del plan)

Este paso desbloquea todo lo demás.

1. Hacer que `Application` posea un `IRenderer2D`.
2. Construirlo con el renderer SDL que expone `Window`.
3. Reemplazar el render en `Window::OnUpdate()` por llamadas al renderer.
4. Verificar que la ventana sigue mostrando el color de fondo.

Criterio de éxito: `Window::OnUpdate()` ya no hace clear ni present. El
renderer los hace.

### Paso 2 — Primitivas (Fase 1 parcial)

1. Añadir `Rect` a la interfaz pública.
2. Añadir `DrawRect`, `DrawLine`, `DrawCircle` a `IRenderer2D`.
3. Implementar los tres métodos en `SDLRenderer2D`.
4. Probar dibujando formas en pantalla desde el punto de extensión del juego.

### Paso 3 — TextureHandle y DrawSprite (Fase 1)

1. Definir `TextureHandle` como alias de entero y el valor de handle inválido.
2. Añadir `CreateTexture`, `DestroyTexture`, `DrawSprite` a `IRenderer2D`.
3. Implementar en `SDLRenderer2D` con la tabla interna de handles.
4. Probar con datos de píxeles hardcodeados (un cuadrado de color sólido).

### Paso 4 — ResourceManager (Fase 1)

1. Integrar stb_image o SDL_image para decodificar PNG.
2. Implementar `Load` con caché y `Unload`.
3. Conectar `ResourceManager` al renderer.
4. Probar cargando un PNG real desde disco.

### Paso 5 — Camera2D (Fase 2)

1. Definir `Camera2D` con posición y zoom.
2. Añadir `SetCamera`, `BeginScreenSpace`, `EndScreenSpace` a `IRenderer2D`.
3. Añadir `ScreenToWorld` y `WorldToScreen` como helpers.
4. Implementar en `SDLRenderer2D`.
5. Probar moviendo la cámara con teclas.

---

*Última actualización: 2026-05-07*
