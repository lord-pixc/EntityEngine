#include "../../include/Platform/Input.h"

#include <SDL3/SDL.h>

namespace EntityEngine
{

    std::array<bool, SDL_SCANCODE_COUNT> Input::s_Keys{};
    std::array<bool, 5> Input::s_MouseButtons{};
    int Input::s_MouseX = 0;
    int Input::s_MouseY = 0;

    void Input::OnEvent(const SDL_Event &event)
    {
        switch (event.type)
        {
        case SDL_EVENT_KEY_DOWN:
        {
            auto scancode = event.key.scancode;
            if (scancode >= 0 && scancode < SDL_SCANCODE_COUNT)
            {
                s_Keys[scancode] = true;
            }
            break;
        }
        case SDL_EVENT_KEY_UP:
        {
            auto scancode = event.key.scancode;
            if (scancode >= 0 && scancode < SDL_SCANCODE_COUNT)
            {
                s_Keys[scancode] = false;
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

    bool Input::IsKeyPressed(SDL_Scancode key)
    {
        if (key < 0 || key >= SDL_SCANCODE_COUNT)
            return false;
        return s_Keys[key];
    }

    bool Input::IsMouseButtonPressed(std::uint8_t button)
    {
        if (button >= s_MouseButtons.size())
            return false;
        return s_MouseButtons[button];
    }

    int Input::GetMouseX()
    {
        return s_MouseX;
    }

    int Input::GetMouseY()
    {
        return s_MouseY;
    }

} // namespace EntityEngine
