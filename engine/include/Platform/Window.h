#pragma once

#include <string>

// Forward declaration para no incluir SDL en el header público del engine
struct SDL_Window;
struct SDL_Renderer;

namespace EntityEngine
{

    class Window
    {
    public:
        Window(const std::string &title, int width, int height);
        ~Window();

        bool IsValid() const { return m_Window != nullptr; }

        void OnUpdate();

        int GetWidth() const { return m_Width; }
        int GetHeight() const { return m_Height; }

        void UpdateSize(int width, int height);

        // Punteros para OpenGL, etc.
        SDL_Window *GetNativeWindow() const { return m_Window; }
        SDL_Renderer *GetRenderer() const { return m_Renderer; }

    private:
        SDL_Window *m_Window;
        SDL_Renderer *m_Renderer;
        int m_Width;
        int m_Height;
    };

} // namespace EntityEngine
