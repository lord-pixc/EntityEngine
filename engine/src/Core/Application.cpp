#include "../../include/Core/Application.h"
#include "../../include/Core/Game.h"
#include "../../include/Core/Time.h"
#include "../../include/Core/log.h"
#include "../../include/Platform/Input.h"
#include "../../include/Platform/Window.h"
#include "../../include/Render/Backend/SDLRenderer2D.h"

#include <SDL3/SDL.h>

namespace EntityEngine {

Application::Application(const std::string &title, int width, int height)
    : m_IsRunning(false), m_SdlInitialized(false) {
    EE_LOG_TRACE("Inicializando Application...");

    // Inicializa unicamente el subsistema de vídeo.
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        EE_LOG_ERROR(std::string("Error inicializando SDL: ") + SDL_GetError());
        return;
    }

    m_SdlInitialized = true;
    EE_LOG_TRACE("SDL_Init successful, continuing setup.");
    EE_LOG_INFO("SDL inicializado OK");

    // Crear la ventana principal con renderizador asociado.
    m_Window = std::make_unique<Window>(title, width, height);
    if (!m_Window || !m_Window->IsValid()) {
        EE_LOG_ERROR(" No se pudo crear la ventana.");
        return;
    }
    EE_LOG_INFO("Ventana creada OK");

    m_Renderer2D = std::make_unique<SDLRenderer2D>(m_Window->GetRenderer());

    // Activa el reloj para medir el delta time desde el primer frame.
    Time::Init();
    m_IsRunning = true;
    EE_LOG_TRACE("Application lista, entrando a Run()...");
}

Application::~Application() {
    m_Renderer2D.reset();
    m_Window.reset();

    if (m_SdlInitialized) {
        SDL_Quit();
    }
}

void Application::Run() {
    Game game;
    Run(game);
}

void Application::Run(Game &game) {
    if (!m_IsRunning)
        return;

    EE_LOG_TRACE("Application loop started.");
    game.OnStart();

    while (m_IsRunning) {
        // Calcula el tiempo transcurrido desde el último frame para las
        // actualizaciones dependientes del tiempo.
        Time::Update();
        const float deltaTime = Time::GetDeltaTime();

        // Manejo de eventos
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
             // Propaga los eventos a los sistemas de entrada.
            Input::OnEvent(&event);

            if (event.type == SDL_EVENT_QUIT) {
                m_IsRunning = false;
            }

            if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                const int newWidth = event.window.data1;
                const int newHeight = event.window.data2;
                if (m_Window) {
                    m_Window->UpdateSize(newWidth, newHeight);
                }
            }
        }

        game.OnUpdate(deltaTime);

        m_Renderer2D->BeginFrame();
        m_Renderer2D->Clear({20, 20, 25, 255});

        game.OnRender(*m_Renderer2D);

        m_Renderer2D->EndFrame();

        Input::EndFrame();
    }

    game.OnShutdown();
}

} // namespace EntityEngine
