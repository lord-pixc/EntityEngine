#pragma once

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include "InputCodes.h"

namespace EntityEngine
{

    struct KeyBinding
    {
        KeyCode Key;

        bool operator==(const KeyBinding&) const = default;
    };

    struct MouseButtonBinding
    {
        MouseButton Button;

        bool operator==(const MouseButtonBinding&) const = default;
    };

    using InputBinding = std::variant<KeyBinding, MouseButtonBinding>;

    /**
     * @brief Mapa de entrada que asocia acciones del juego a teclas o botones.
     *
     * Permite definir acciones abstractas (como "Jump" o "Shoot") y asignarles
     * combinaciones de teclas o botones. Facilita la reconfiguración de controles
     * sin cambiar el código del juego, y soporta múltiples dispositivos de entrada.
     */
    class InputMap
    {
    public:
        void Bind(const std::string &action, KeyCode key);
        void Bind(const std::string &action, MouseButton button);

        bool HasAction(const std::string &action) const;
        bool ClearAction(const std::string &action);
        bool ReplaceBinding(const std::string &action, const InputBinding &oldBinding, const InputBinding &newBinding);

        bool IsActionHeld(const std::string &action) const;
        bool IsActionJustPressed(const std::string &action) const;
        bool IsActionJustReleased(const std::string &action) const;

    private:
        // Mapas internos para asociar acciones a teclas y botones.
        std::unordered_map<std::string, std::vector<InputBinding>> m_Actions;
    };

} // namespace EntityEngine