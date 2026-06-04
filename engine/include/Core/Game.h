#pragma once

namespace EntityEngine
{
    class IRenderer2D;

    /**
     * @brief Interfaz base para conectar lógica de juego al motor.
     */
    class Game
    {
    public:
        virtual ~Game() = default;

        virtual void OnStart() {}
        virtual void OnUpdate(float deltaTime) {}
        virtual void OnRender(IRenderer2D &renderer) {}
        virtual void OnShutdown() {}
    };
}
