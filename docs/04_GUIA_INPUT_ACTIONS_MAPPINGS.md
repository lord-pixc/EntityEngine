# Guia para implementar Input Actions y Mappings

Esta guia explica que son las acciones de input, que es un mapping, por que
existen y como implementarlos encima del sistema `Input` actual del motor.

El objetivo es que el codigo de juego deje de preguntar directamente por teclas
fisicas:

```cpp
if (Input::IsKeyJustPressed(KeyCode::Space))
{
    player.Jump();
}
```

Y empiece a preguntar por intenciones de gameplay:

```cpp
if (inputMap.IsActionJustPressed("Jump"))
{
    player.Jump();
}
```

---

## 1. Punto de partida: input fisico

El sistema `Input` actual responde preguntas sobre dispositivos fisicos:

```cpp
Input::IsKeyHeld(KeyCode::A);
Input::IsKeyJustPressed(KeyCode::Space);
Input::IsMouseButtonHeld(MouseButton::Left);
```

Eso significa:

- "La tecla fisica A esta presionada?"
- "La tecla fisica Space se acaba de presionar?"
- "El boton izquierdo del mouse esta presionado?"

Esta capa es necesaria, pero no es ideal para escribir gameplay directamente.

El gameplay normalmente no deberia pensar en teclas. Deberia pensar en
acciones del jugador:

```txt
Jump
Attack
Pause
MoveLeft
OpenInventory
```

A esas intenciones las llamamos **Input Actions**.

---

## 2. Que es una Action

Una **Action** es una accion de gameplay nombrada por el juego.

Ejemplos:

```txt
Jump
Attack
Pause
OpenInventory
ConfirmDialogue
ToggleBuildMode
```

El motor no deberia decidir que acciones existen. Eso lo decide cada juego.

Por eso no conviene hacer esto en un motor general:

```cpp
enum class Action
{
    Jump,
    Attack,
    Dash,
    Interact
};
```

Ese enum encerraria al motor en un tipo especifico de juego.

Es mejor empezar con nombres definidos por el game dev:

```cpp
inputMap.Bind("Jump", KeyCode::Space);
inputMap.Bind("OpenInventory", KeyCode::I);
inputMap.Bind("Attack", MouseButton::Left);
```

El motor solo sabe que existe una accion llamada `"Jump"`. No sabe ni necesita
saber que significa saltar.

---

## 3. Que es un Binding

Un **Binding** es una conexion entre una action y una entrada fisica.

Ejemplo:

```txt
Action          Binding fisico
------          --------------
Jump            Space
Attack          Mouse Left
Pause           Escape
OpenInventory   I
```

Una action puede tener varios bindings:

```txt
Jump -> Space
Jump -> W
Jump -> Gamepad A
```

Entonces `"Jump"` se activa si cualquiera de sus bindings se activa.

En pseudocodigo:

```cpp
IsActionJustPressed("Jump")
{
    revisar todos los bindings de Jump

    si alguno acaba de presionarse:
        return true

    return false
}
```

---

## 4. Que es un Mapping

Un **InputMap** es la tabla que guarda que bindings pertenecen a cada action.

Ejemplo mental:

```txt
"Jump"   -> [Space, W]
"Attack" -> [Mouse Left, LeftCtrl]
"Pause"  -> [Escape]
```

En C++, esta estructura puede representarse con:

```cpp
std::unordered_map<std::string, std::vector<InputBinding>> m_Actions;
```

Eso significa:

```txt
nombre de accion -> lista de bindings
```

---

## 5. Por que esto sirve para remapeo

Sin mappings, el gameplay suele terminar acoplado a teclas fisicas:

```cpp
if (Input::IsKeyJustPressed(KeyCode::Space))
{
    player.Jump();
}
```

Si despues el jugador quiere cambiar salto a `W`, habria que tocar codigo o
agregar condiciones:

```cpp
if (Input::IsKeyJustPressed(KeyCode::Space) ||
    Input::IsKeyJustPressed(KeyCode::W))
{
    player.Jump();
}
```

Con `InputMap`, el gameplay no cambia:

```cpp
if (inputMap.IsActionJustPressed("Jump"))
{
    player.Jump();
}
```

Solo cambia la configuracion:

```cpp
inputMap.ClearAction("Jump");
inputMap.Bind("Jump", KeyCode::W);
```

Esto permite:

- remapear controles sin tocar gameplay;
- tener varios bindings por accion;
- agregar presets de teclado;
- preparar soporte futuro para gamepad;
- guardar y cargar configuraciones del jugador.

---

## 6. Held, JustPressed y JustReleased

Una action debe poder consultarse de varias formas.

### Held

Significa: "esta activa mientras el input se mantiene presionado".

Sirve para movimiento continuo:

```cpp
if (inputMap.IsActionHeld("MoveRight"))
{
    player.x += speed * dt;
}
```

### JustPressed

Significa: "se activo justo este frame".

Sirve para acciones que deben ejecutarse una vez:

```cpp
if (inputMap.IsActionJustPressed("Jump"))
{
    player.Jump();
}
```

### JustReleased

Significa: "se solto justo este frame".

Sirve para acciones como soltar un disparo cargado:

```cpp
if (inputMap.IsActionJustReleased("ChargeShot"))
{
    player.FireChargedShot();
}
```

---

## 7. Primer diseno recomendado

Para una primera implementacion, conviene soportar solo acciones binarias:

- teclado;
- botones de mouse;
- multiples bindings por action;
- consultas `Held`, `JustPressed`, `JustReleased`.

No conviene empezar todavia con:

- ejes analogicos;
- gamepad;
- combinaciones como `Ctrl + S`;
- contextos como `Gameplay`, `Menu`, `Inventory`;
- serializacion JSON;
- input buffering.

Todo eso puede agregarse despues encima de una base simple.

---

## 8. Estructura de archivos sugerida

Mantener las capas separadas:

```txt
engine/include/Platform/InputCodes.h
    KeyCode, MouseButton

engine/include/Platform/Input.h
engine/src/Platform/Input.cpp
    estado crudo de teclado y mouse

engine/include/Platform/InputMap.h
engine/src/Platform/InputMap.cpp
    actions, bindings y consultas semanticas
```

`Input` sabe:

```txt
Space esta presionado
Mouse Left se solto
```

`InputMap` sabe:

```txt
Jump esta vinculado a Space
Attack esta vinculado a Mouse Left
```

El juego sabe:

```txt
si Jump, llamar player.Jump()
si Attack, llamar player.Attack()
```

---

## 9. Representar un binding

Una forma facil de entender es usar un enum de tipo:

```cpp
enum class InputBindingType
{
    Key,
    MouseButton
};
```

Y un struct:

```cpp
struct InputBinding
{
    InputBindingType type;
    KeyCode key = KeyCode::Unknown;
    MouseButton mouseButton = MouseButton::Left;
};
```

Si `type == InputBindingType::Key`, se usa `key`.

Si `type == InputBindingType::MouseButton`, se usa `mouseButton`.

Ejemplo para teclado:

```cpp
InputBinding jumpBinding;
jumpBinding.type = InputBindingType::Key;
jumpBinding.key = KeyCode::Space;
```

Ejemplo para mouse:

```cpp
InputBinding attackBinding;
attackBinding.type = InputBindingType::MouseButton;
attackBinding.mouseButton = MouseButton::Left;
```

Mas adelante se puede usar `std::variant` para un diseno mas expresivo, pero el
enum `type` es mas facil de implementar al inicio.

---

## 10. API inicial de InputMap

Una primera version razonable:

```cpp
#pragma once

#include "InputCodes.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace EntityEngine
{
    enum class InputBindingType
    {
        Key,
        MouseButton
    };

    struct InputBinding
    {
        InputBindingType type;
        KeyCode key = KeyCode::Unknown;
        MouseButton mouseButton = MouseButton::Left;

        static InputBinding FromKey(KeyCode key);
        static InputBinding FromMouseButton(MouseButton button);
    };

    class InputMap
    {
    public:
        void Bind(const std::string& action, KeyCode key);
        void Bind(const std::string& action, MouseButton button);

        void ClearAction(const std::string& action);
        bool HasAction(const std::string& action) const;

        bool IsActionHeld(const std::string& action) const;
        bool IsActionJustPressed(const std::string& action) const;
        bool IsActionJustReleased(const std::string& action) const;

    private:
        std::unordered_map<std::string, std::vector<InputBinding>> m_Actions;
    };
}
```

---

## 11. Como funciona Bind

Cuando el game dev escribe:

```cpp
inputMap.Bind("Jump", KeyCode::Space);
```

Internamente deberia pasar algo como:

```cpp
m_Actions["Jump"].push_back(InputBinding::FromKey(KeyCode::Space));
```

Si `"Jump"` no existe, `unordered_map` crea la entrada.

Despues queda:

```txt
Jump -> [Space]
```

Si luego se llama:

```cpp
inputMap.Bind("Jump", KeyCode::W);
```

queda:

```txt
Jump -> [Space, W]
```

---

## 12. Como funciona IsActionJustPressed

Cuando el juego llama:

```cpp
inputMap.IsActionJustPressed("Jump");
```

El algoritmo es:

1. Buscar `"Jump"` en `m_Actions`.
2. Si no existe, devolver `false`.
3. Recorrer todos sus bindings.
4. Si el binding es una tecla, consultar `Input::IsKeyJustPressed(binding.key)`.
5. Si el binding es un boton de mouse, consultar `Input::IsMouseButtonJustPressed(binding.mouseButton)`.
6. Si cualquiera devuelve `true`, la action devuelve `true`.
7. Si ninguno devuelve `true`, devolver `false`.

Pseudocodigo:

```cpp
bool InputMap::IsActionJustPressed(const std::string& action) const
{
    auto it = m_Actions.find(action);

    if (it == m_Actions.end())
    {
        return false;
    }

    const auto& bindings = it->second;

    for (const auto& binding : bindings)
    {
        if (binding.type == InputBindingType::Key)
        {
            if (Input::IsKeyJustPressed(binding.key))
            {
                return true;
            }
        }

        if (binding.type == InputBindingType::MouseButton)
        {
            if (Input::IsMouseButtonJustPressed(binding.mouseButton))
            {
                return true;
            }
        }
    }

    return false;
}
```

`IsActionHeld` y `IsActionJustReleased` siguen la misma idea, pero llaman a:

```cpp
Input::IsKeyHeld(...)
Input::IsKeyJustReleased(...)
Input::IsMouseButtonHeld(...)
Input::IsMouseButtonJustReleased(...)
```

---

## 13. Remapear controles

Remapear significa cambiar que input fisico activa una action.

Ejemplo:

```txt
antes:   Jump -> Space
despues: Jump -> W
```

Una forma simple:

```cpp
inputMap.ClearAction("Jump");
inputMap.Bind("Jump", KeyCode::W);
```

Mas adelante se puede agregar:

```cpp
ReplaceBinding("Jump", oldBinding, newBinding);
RemoveBinding("Jump", binding);
```

Pero no es necesario para la primera version.

---

## 14. Ejes: dejar para despues

Los ejes son acciones con valor numerico, por ejemplo:

```cpp
float x = inputMap.GetAxis("MoveX");
```

Un eje digital podria mapearse asi:

```cpp
inputMap.BindAxis("MoveX", KeyCode::A, KeyCode::D);
```

Donde:

```txt
A -> -1
D -> +1
ninguna -> 0
ambas -> 0
```

Pero los ejes tienen mas decisiones:

- que pasa si ambas teclas estan presionadas;
- como se mezclan teclado y gamepad;
- dead zones;
- sensibilidad;
- smoothing.

Por eso conviene implementarlos despues de las acciones binarias.

---

## 15. Orden recomendado de implementacion

1. Crear `InputMap.h` y `InputMap.cpp`.
2. Crear `InputBindingType` e `InputBinding`.
3. Implementar `Bind(action, KeyCode)`.
4. Implementar `Bind(action, MouseButton)`.
5. Implementar `ClearAction`.
6. Implementar `HasAction`.
7. Implementar `IsActionHeld`.
8. Implementar `IsActionJustPressed`.
9. Implementar `IsActionJustReleased`.
10. Probar con una action como `"Jump"` y otra como `"Attack"`.
11. Despues pensar en ejes digitales.
12. Despues pensar en serializacion para guardar controles.
13. Despues pensar en gamepad.

---

## 16. Frase clave

El motor no sabe que es saltar.

El motor solo sabe:

```txt
existe una action llamada "Jump"
"Jump" esta conectada a KeyCode::Space
KeyCode::Space se acaba de presionar
por lo tanto "Jump" se acaba de activar
```

El juego decide que hacer con eso:

```cpp
if (inputMap.IsActionJustPressed("Jump"))
{
    player.Jump();
}
```
