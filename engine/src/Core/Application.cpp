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

        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
        {
            std::cerr << "Error initializing SDL: " << SDL_GetError() << std::endl;
            return;
        }

        // Crear la ventana
        m_Window = std::make_unique<Window>(title, width, height);
        if (!m_Window->IsValid())
        {
            std::cerr << "Failed to create window." << std::endl;
            return;
        }

        Time::Init();
        m_IsRunning = true;
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