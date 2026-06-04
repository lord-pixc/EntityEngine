#include "Core/Application.h"
#include "Core/Game.h"
#include "Render/IRenderer2D.h"

#include <iostream>

class DemoGame : public EntityEngine::Game
{
public:
    void OnStart() override
    {
        std::cout << "[Game] Demo iniciada.\n";
    }

    void OnUpdate(float deltaTime) override
    {
        (void)deltaTime;
    }

    void OnRender(EntityEngine::IRenderer2D &renderer) override
    {
        renderer.DrawRect({80.0f, 80.0f, 240.0f, 140.0f}, {220, 80, 70, 255}, true);
        renderer.DrawRect({360.0f, 120.0f, 220.0f, 120.0f}, {80, 170, 220, 255}, false);
        renderer.DrawLine({80.0f, 280.0f, 580.0f, 320.0f}, {240, 220, 120, 255});
        renderer.DrawCircle({720.0f, 180.0f, 70.0f}, {120, 220, 150, 255}, false);
    }

    void OnShutdown() override
    {
        std::cout << "[Game] Demo finalizada.\n";
    }
};

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    // Punto de entrada del ejecutable de ejemplo. Inicializa la aplicación del
    // motor, ejecuta su bucle principal y luego sale limpiamente.
    std::cout << "[Game] Iniciando EntityEngine...\n";

    EntityEngine::Application app("EntityEngine Demo", 1280, 720);
    DemoGame game;
    app.Run(game);

    std::cout << "[Game] Saliendo.";
}
