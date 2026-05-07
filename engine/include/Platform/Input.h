#pragma once

#include <cstdint>
#include <array>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_events.h>

namespace EntityEngine
{

    /**
     * @brief Sistema estático para consultar el estado de teclado y ratón.
     *
     * Debe alimentarse con los eventos SDL recibidos en el loop principal
     * mediante OnEvent. Posteriormente puede consultarse en cualquier parte
     * del código para saber qué teclas o botones están presionados y la
     * posición actual del cursor.
     */
    class Input
    {
    public:
        // Llamado por Application para alimentar el sistema de input
        static void OnEvent(const SDL_Event &event);

        static void EndFrame();

        // Teclado
        static bool IsKeyHeld(SDL_Scancode key);
        static bool IsKeyJustPressed(SDL_Scancode key);
        static bool IsKeyJustReleased(SDL_Scancode key);

        // Mouse
        static bool IsMouseButtonHeld(std::uint8_t button);
        static bool IsMouseButtonJustPressed(std::uint8_t button);
        static bool IsMouseButtonJustReleased(std::uint8_t button);
        static int GetMouseX();
        static int GetMouseY();

    private:
        static std::array<bool, SDL_SCANCODE_COUNT> s_CurrentKeys;
        static std::array<bool, SDL_SCANCODE_COUNT> s_PrevKeys;
        static std::array<bool, 6> s_MouseButtons; // izquierda, medio, derecha, etc.
        static std::array<bool, 6> s_PrevMouseButtons;
        static int s_MouseX;
        static int s_MouseY;
    };

} // namespace EntityEngine
