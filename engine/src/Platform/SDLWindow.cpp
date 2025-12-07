#include "../../include/Platform/Window.h"

#include <SDL3/SDL.h>
#include <iostream>

namespace EntityEngine
{

    Window::Window(const std::string &title, int width, int height)
        : m_Window(nullptr), m_Renderer(nullptr),
          m_Width(width), m_Height(height)
    {

        m_Window = SDL_CreateWindow(title.c_str(), width, height,
                                    SDL_WINDOW_RESIZABLE);
        if (!m_Window)
        {
            std::cerr << "[EntityEngine] Error creando SDL_Window: "
                      << SDL_GetError() << std::endl;
            return;
        }

        // Renderer de SDL por ahora
        m_Renderer = SDL_CreateRenderer(m_Window, nullptr);
        if (!m_Renderer)
        {
            std::cerr << "[EntityEngine] Error creando SDL_Renderer: "
                      << SDL_GetError() << std::endl;
            SDL_DestroyWindow(m_Window);
            m_Window = nullptr;
            return;
        }

        // Activar VSync
        SDL_SetRenderVSync(m_Renderer, 1);
    }

    Window::~Window()
    {
        if (m_Renderer)
        {
            SDL_DestroyRenderer(m_Renderer);
            m_Renderer = nullptr;
        }
        if (m_Window)
        {
            SDL_DestroyWindow(m_Window);
            m_Window = nullptr;
        }
    }

    void Window::OnUpdate()
    {
        if (!m_Renderer)
            return;

        // Limpiar la pantalla con un color de fondo simple
        SDL_SetRenderDrawColor(m_Renderer, 20, 20, 25, 255);
        SDL_RenderClear(m_Renderer);

        // Aquí luego vendrán las llamadas de Render2D (sprites, etc.)

        SDL_RenderPresent(m_Renderer);
    }

} // namespace EntityEngine
