#include "../../include/Platform/Input.h"

#include <SDL3/SDL.h>

namespace EntityEngine
{

    static_assert(static_cast<int>(KeyCode::Unknown) == SDL_SCANCODE_UNKNOWN);
    static_assert(static_cast<int>(KeyCode::A) == SDL_SCANCODE_A);
    static_assert(static_cast<int>(KeyCode::Z) == SDL_SCANCODE_Z);
    static_assert(static_cast<int>(KeyCode::Num1) == SDL_SCANCODE_1);
    static_assert(static_cast<int>(KeyCode::Num0) == SDL_SCANCODE_0);
    static_assert(static_cast<int>(KeyCode::Escape) == SDL_SCANCODE_ESCAPE);
    static_assert(static_cast<int>(KeyCode::Space) == SDL_SCANCODE_SPACE);
    static_assert(static_cast<int>(KeyCode::F1) == SDL_SCANCODE_F1);
    static_assert(static_cast<int>(KeyCode::F24) == SDL_SCANCODE_F24);
    static_assert(static_cast<int>(KeyCode::LeftCtrl) == SDL_SCANCODE_LCTRL);
    static_assert(static_cast<int>(KeyCode::RightGui) == SDL_SCANCODE_RGUI);
    static_assert(static_cast<int>(KeyCode::MediaPlay) == SDL_SCANCODE_MEDIA_PLAY);
    static_assert(static_cast<int>(KeyCode::AcBookmarks) == SDL_SCANCODE_AC_BOOKMARKS);
    static_assert(static_cast<int>(KeyCode::EndCall) == SDL_SCANCODE_ENDCALL);
    static_assert(static_cast<int>(KeyCode::Count) == SDL_SCANCODE_COUNT);

    static_assert(static_cast<int>(MouseButton::Left) == SDL_BUTTON_LEFT);
    static_assert(static_cast<int>(MouseButton::Middle) == SDL_BUTTON_MIDDLE);
    static_assert(static_cast<int>(MouseButton::Right) == SDL_BUTTON_RIGHT);
    static_assert(static_cast<int>(MouseButton::X1) == SDL_BUTTON_X1);
    static_assert(static_cast<int>(MouseButton::X2) == SDL_BUTTON_X2);

    std::array<bool, Input::KeyCount> Input::s_CurrentKeys{};
    std::array<bool, Input::KeyCount> Input::s_PrevKeys{};
    std::array<bool, Input::MouseButtonCount> Input::s_MouseButtons{};
    std::array<bool, Input::MouseButtonCount> Input::s_PrevMouseButtons{};
    int Input::s_MouseX = 0;
    int Input::s_MouseY = 0;

    void Input::OnEvent(const void *nativeEvent)
    {
        if (!nativeEvent)
        {
            return;
        }

        const auto &event = *static_cast<const SDL_Event *>(nativeEvent);

        // Actualiza el estado interno en función del tipo de evento recibido
        // para que las consultas estáticas reflejen la situación más reciente.
        switch (event.type)
        {
        case SDL_EVENT_KEY_DOWN:
        {
            auto scancode = event.key.scancode;
            if (scancode >= 0 && static_cast<std::size_t>(scancode) < s_CurrentKeys.size())
            {
                s_CurrentKeys[static_cast<std::size_t>(scancode)] = true;
            }
            break;
        }
        case SDL_EVENT_KEY_UP:
        {
            auto scancode = event.key.scancode;
            if (scancode >= 0 && static_cast<std::size_t>(scancode) < s_CurrentKeys.size())
            {
                s_CurrentKeys[static_cast<std::size_t>(scancode)] = false;
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
    bool Input::IsKeyHeld(KeyCode key)
    {
        const auto index = static_cast<std::size_t>(key);
        if (index >= s_CurrentKeys.size())
            return false;
        return s_CurrentKeys[index];
    }

    bool Input::IsKeyJustPressed(KeyCode key)
    {
        const auto index = static_cast<std::size_t>(key);
        if (index >= s_CurrentKeys.size())
            return false;
        return s_CurrentKeys[index] && !s_PrevKeys[index];
    }

    bool Input::IsKeyJustReleased(KeyCode key)
    {
        const auto index = static_cast<std::size_t>(key);
        if (index >= s_CurrentKeys.size())
            return false;
        return !s_CurrentKeys[index] && s_PrevKeys[index];
    }


    // Mouse
    bool Input::IsMouseButtonHeld(MouseButton button)
    {
        const auto index = static_cast<std::size_t>(button);
        if (index >= s_MouseButtons.size())
            return false;
        return s_MouseButtons[index];
    }

    bool Input::IsMouseButtonJustPressed(MouseButton button)
    {
        const auto index = static_cast<std::size_t>(button);
        if (index >= s_MouseButtons.size())
            return false;
        return s_MouseButtons[index] && !s_PrevMouseButtons[index];
    }

    bool Input::IsMouseButtonJustReleased(MouseButton button)
    {
        const auto index = static_cast<std::size_t>(button);
        if (index >= s_MouseButtons.size())
            return false;
        return !s_MouseButtons[index] && s_PrevMouseButtons[index];
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
