#pragma once

#include <array>
#include <cstddef>

#include "InputCodes.h"

namespace EntityEngine
{

    /**
     * @brief Sistema estático para consultar el estado de teclado y ratón.
     *
     * La API pública usa tipos propios del motor para no filtrar SDL al código
     * de juego. Application alimenta este sistema desde el backend activo.
     */
    class Input
    {
    public:
        // Teclado
        static bool IsKeyHeld(KeyCode key);
        static bool IsKeyJustPressed(KeyCode key);
        static bool IsKeyJustReleased(KeyCode key);

        // Mouse
        static bool IsMouseButtonHeld(MouseButton button);
        static bool IsMouseButtonJustPressed(MouseButton button);
        static bool IsMouseButtonJustReleased(MouseButton button);
        static int GetMouseX();
        static int GetMouseY();

    private:
        friend class Application;

        static constexpr std::size_t KeyCount = static_cast<std::size_t>(KeyCode::Count);
        static constexpr std::size_t MouseButtonCount = 6; // SDL reserva el índice 0 y usa 1..5.

        static void OnEvent(const void *event);
        static void EndFrame();

        static std::array<bool, KeyCount> s_CurrentKeys;
        static std::array<bool, KeyCount> s_PrevKeys;
        static std::array<bool, MouseButtonCount> s_MouseButtons;
        static std::array<bool, MouseButtonCount> s_PrevMouseButtons;
        static int s_MouseX;
        static int s_MouseY;
    };

} // namespace EntityEngine
