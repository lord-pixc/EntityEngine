#include "../../include/Core/Application.h"
#include "../../include/Platform/Window.h"
#include "../../include/Core/Time.h"
#include "../../include/Platform/Input.h"

#include <SDL3/SDL.h>
#include <iostream>

namespace EntityEngine
{

    Application::Application(const std::string &title, int width, int height)
        : m_IsRunning(false), m_SdlInitialized(false)
    {
        std::cout << "[EntityEngine] Iniciando SDL...\n";

        // Inicializa únicamente el subsistema de vídeo.
        if (!SDL_Init(SDL_INIT_VIDEO))
        {
            std::cerr << "[EntityEngine] Error inicializando SDL: "
                      << SDL_GetError() << std::endl;
            return;
        }

        m_SdlInitialized = true;
        std::cout << "[EntityEngine] SDL_Init successful, continuing setup.\n";
        std::cout << "[EntityEngine] SDL inicializado OK\n";

        // Crear la ventana principal con renderizador asociado.
        m_Window = std::make_unique<Window>(title, width, height);
        if (!m_Window || !m_Window->IsValid())
        {
            std::cerr << "[EntityEngine] No se pudo crear la ventana.\n";
            return;
        }

        std::cout << "[EntityEngine] Ventana creada OK\n";

        // Activa el reloj para medir el delta time desde el primer frame.
        Time::Init();
        m_IsRunning = true;
        std::cout << "[EntityEngine] Application lista, entrando a Run()...\n";
    }

    Application::~Application()
    {
        if (m_SdlInitialized)
        {
            SDL_Quit();
        }
    }

    void Application::Run()
    {
        if (!m_IsRunning)
            return;

        std::cout << "Application loop started." << std::endl;

        while (m_IsRunning)
        {
            // Calcula el tiempo transcurrido desde el último frame para las
            // actualizaciones dependientes del tiempo.
            Time::Update();
            const float deltaTime = Time::GetDeltaTime();

            // Manejo de eventos
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                // Propaga los eventos a los sistemas de entrada.
                Input::OnEvent(event);

                if (event.type == SDL_EVENT_QUIT)
                {
                    m_IsRunning = false;
                }

                if (event.type == SDL_EVENT_WINDOW_RESIZED)
                {
                    const int newWidth = event.window.data1;
                    const int newHeight = event.window.data2;
                    if (m_Window)
                    {
                        m_Window->UpdateSize(newWidth, newHeight);
                    }
                }
            }

            // Actualización de la aplicación / logica del motor
            OnUpdate(deltaTime);
            m_Window->OnUpdate();
        }
    }

    void Application::OnUpdate(float deltaTime)
    {
        // Actualizar escenas, entidades, etc. (se implementará más adelante).
    }

} // namespace EntityEngine
