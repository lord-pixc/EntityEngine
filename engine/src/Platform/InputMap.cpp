#include "../../include/Platform/InputMap.h"
#include "../../include/Platform/Input.h"
#include "../../include/Core/log.h"

namespace EntityEngine
{

    void InputMap::Bind(const std::string &action, KeyCode key)
    {
        m_Actions[action].push_back(InputBinding(KeyBinding{key}));
    }

    void InputMap::Bind(const std::string &action, MouseButton button)
    {
        m_Actions[action].push_back(InputBinding(MouseButtonBinding{button}));
    }

    bool InputMap::HasAction(const std::string &action) const
    {
        return m_Actions.find(action) != m_Actions.end();
    }

    bool InputMap::ClearAction(const std::string &action)
    {
        auto actionIt = m_Actions.find(action);
        if (actionIt != m_Actions.end())
        {
            m_Actions.erase(actionIt);
            return true;
        }

        EE_LOG_TRACE("Action not found for ClearAction: " + action);
        return false; // Acción no encontrada
    }

    bool InputMap::ReplaceBinding(const std::string &action, const InputBinding &oldBinding, const InputBinding &newBinding)
    {
        auto actionIt = m_Actions.find(action);
        if (actionIt != m_Actions.end())
        {
            auto &bindings = actionIt->second;
            for (auto &binding : bindings)
            {
                if (binding == oldBinding)
                {
                    binding = newBinding;
                    return true; // Reemplazo exitoso
                }
            }
            EE_LOG_TRACE("Old binding not found for ReplaceBinding: " + action);
            return false; // Old binding no encontrado
        }
        EE_LOG_TRACE("Action not found for ReplaceBinding: " + action);
        return false; // Acción no encontrada
    }

    bool InputMap::IsActionHeld(const std::string &action) const
    {
        auto it = m_Actions.find(action);
        if (it != m_Actions.end())
        {
            const auto &bindings = it->second;
            for (const auto &binding : bindings)
            {
                if (std::holds_alternative<KeyBinding>(binding))
                {
                    const auto &keyBinding = std::get<KeyBinding>(binding);
                    if (Input::IsKeyHeld(keyBinding.Key))
                        return true;
                }
                else if (std::holds_alternative<MouseButtonBinding>(binding))
                {
                    const auto &mouseBinding = std::get<MouseButtonBinding>(binding);
                    if (Input::IsMouseButtonHeld(mouseBinding.Button))
                        return true;
                }
            }
            return false; // Ningún binding activo
        }

        EE_LOG_TRACE("Action not found for IsActionHeld: " + action);
        return false; // Acción no encontrada
    }

    bool InputMap::IsActionJustPressed(const std::string &action) const
    {
        auto it = m_Actions.find(action);
        if (it != m_Actions.end())
        {
            const auto &bindings = it->second;
            for (const auto &binding : bindings)
            {
                if (std::holds_alternative<KeyBinding>(binding))
                {
                    const auto &keyBinding = std::get<KeyBinding>(binding);
                    if (Input::IsKeyJustPressed(keyBinding.Key))
                        return true;
                }
                else if (std::holds_alternative<MouseButtonBinding>(binding))
                {
                    const auto &mouseBinding = std::get<MouseButtonBinding>(binding);
                    if (Input::IsMouseButtonJustPressed(mouseBinding.Button))
                        return true;
                }
            }
            return false; // Ningún binding activo
        }

        EE_LOG_TRACE("Action not found for IsActionJustPressed: " + action);
        return false; // Acción no encontrada
    }

    bool InputMap::IsActionJustReleased(const std::string &action) const
    {
        auto it = m_Actions.find(action);
        if (it != m_Actions.end())
        {
            const auto &bindings = it->second;
            for (const auto &binding : bindings)
            {
                if (std::holds_alternative<KeyBinding>(binding))
                {
                    const auto &keyBinding = std::get<KeyBinding>(binding);
                    if (Input::IsKeyJustReleased(keyBinding.Key))
                        return true;
                }
                else if (std::holds_alternative<MouseButtonBinding>(binding))
                {
                    const auto &mouseBinding = std::get<MouseButtonBinding>(binding);
                    if (Input::IsMouseButtonJustReleased(mouseBinding.Button))
                        return true;
                }
            }
            return false; // Ningún binding activo
        }

        EE_LOG_TRACE("Action not found for IsActionJustReleased: " + action);
        return false; // Acción no encontrada
    }

} // namespace EntityEngine