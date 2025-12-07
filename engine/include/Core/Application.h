#pragma once

#include <memory>
#include <string>

namespace EntityEngine
{

    class Window;

    class Application
    {
    public:
        Application(const std::string &title = "Entity Engine", int width = 1280, int height = 720);
        ~Application();

        // loop principal de la aplicación
        void Run();

    private:
        bool m_IsRunning;
        std::unique_ptr<Window> m_Window;

        void OnUpdate(float deltaTime);
    };
}