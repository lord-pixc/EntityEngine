#include "../../include/Platform/Input.h"

#include <SDL3/SDL.h>

namespace EntityEngine
{

    std::array<bool, SDL_SCANCODE_COUNT> Input::s_CurrentKeys{};
    std::array<bool, SDL_SCANCODE_COUNT> Input::s_PrevKeys{};
    std::array<bool, 6> Input::s_MouseButtons{};
    std::array<bool, 6> Input::s_PrevMouseButtons{};
    int Input::s_MouseX = 0;
    int Input::s_MouseY = 0;

    void Input::OnEvent(const SDL_Event &event)
    {
        // Actualiza el estado interno en función del tipo de evento recibido
        // para que las consultas estáticas reflejen la situación más reciente.
        switch (event.type)
        {
        case SDL_EVENT_KEY_DOWN:
        {
            auto scancode = event.key.scancode;
            if (scancode >= 0 && scancode < SDL_SCANCODE_COUNT)
            {
                s_CurrentKeys[scancode] = true;
            }
            break;
        }
        case SDL_EVENT_KEY_UP:
        {
            auto scancode = event.key.scancode;
            if (scancode >= 0 && scancode < SDL_SCANCODE_COUNT)
            {
                s_CurrentKeys[scancode] = false;
            }
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        {
            std::uint8_t button = event.button.button;
            if (button < s_MouseButtons.size())
            {
                s_MouseButtons[button] = true;
            }
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_UP:
        {
            std::uint8_t button = event.button.button;
            if (button < s_MouseButtons.size())
            {
                s_MouseButtons[button] = false;
            }
            break;
        }
        case SDL_EVENT_MOUSE_MOTION:
        {
            s_MouseX = event.motion.x;
            s_MouseY = event.motion.y;
            break;
        }
        default:
            break;
        }
    }

    void Input::EndFrame()
    {
        // Al final de cada frame, actualizamos el estado previo para la próxima consulta.
        s_PrevKeys = s_CurrentKeys;
        s_PrevMouseButtons = s_MouseButtons;
    }

    // Teclado
    bool Input::IsKeyHeld(SDL_Scancode key)
    {
        if (key < 0 || key >= SDL_SCANCODE_COUNT)
            return false;
        return s_CurrentKeys[key];
    }

    bool Input::IsKeyJustPressed(SDL_Scancode key)
    {
        if (key < 0 || key >= SDL_SCANCODE_COUNT)
            return false;
        return s_CurrentKeys[key] && !s_PrevKeys[key];
    }

    bool Input::IsKeyJustReleased(SDL_Scancode key)
    {
        if (key < 0 || key >= SDL_SCANCODE_COUNT)
            return false;
        return !s_CurrentKeys[key] && s_PrevKeys[key];
    }


    // Mouse
    bool Input::IsMouseButtonHeld(std::uint8_t button)
    {
        if (button >= s_MouseButtons.size())
            return false;
        return s_MouseButtons[button];
    }

    bool Input::IsMouseButtonJustPressed(std::uint8_t button)
    {
        if (button >= s_MouseButtons.size())
            return false;
        return s_MouseButtons[button] && !s_PrevMouseButtons[button];
    }

    bool Input::IsMouseButtonJustReleased(std::uint8_t button)
    {
        if (button >= s_MouseButtons.size())
            return false;
        return !s_MouseButtons[button] && s_PrevMouseButtons[button];
    }

    // Posicion del mouse
    int Input::GetMouseX()
    {
        return s_MouseX;
    }

    int Input::GetMouseY()
    {
        return s_MouseY;
    }

} // namespace EntityEngine
