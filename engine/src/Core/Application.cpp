#include "../../include/Core/Application.h"
#include "../../include/Platform/Window.h"
#include "../../include/Core/Time.h"
#include "../../include/Platform/Input.h"

#include <SDL3/SDL.h>
#include <iostream>

namespace EntityEngine
{

    Application::Application(const std::string &title, int width, int height)
        : m_IsRunning(false)
    {
        std::cout << "[EntityEngine] Iniciando SDL...\n";

        // SDL_Init devuelve true en éxito, false en error
        if (!SDL_Init(SDL_INIT_VIDEO))
        {
            std::cerr << "[EntityEngine] Error inicializando SDL: "
                      << SDL_GetError() << std::endl;
            return;
        }

        std::cout << "[EntityEngine] SDL inicializado OK\n";

        m_Window = std::make_unique<Window>(title, width, height);
        if (!m_Window || !m_Window->IsValid())
        {
            std::cerr << "[EntityEngine] No se pudo crear la ventana.\n";
            return;
        }

        std::cout << "[EntityEngine] Ventana creada OK\n";

        Time::Init();
        m_IsRunning = true;
        std::cout << "[EntityEngine] Application lista, entrando a Run()...\n";
    }

    Application::~Application()
    {
        SDL_Quit();
    }

    void Application::Run()
    {
        if (!m_IsRunning)
            return;

        std::cout << "Application loop started." << std::endl;

        while (m_IsRunning)
        {
            Time::Update();
            const float deltaTime = Time::GetDeltaTime();

            // Manejo de eventos
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                // Aquí manejamos los de entrada
                Input::OnEvent(event);

                if (event.type == SDL_EVENT_QUIT)
                {
                    m_IsRunning = false;
                }
            }

            // Actualización de la aplicación / logica del motor
            OnUpdate(deltaTime);
            m_Window->OnUpdate();
        }
    }

    void Application::OnUpdate(float deltaTime)
    {
        // Actualizar escenas, entidades, etc.
    }

} // namespace EntityEngine