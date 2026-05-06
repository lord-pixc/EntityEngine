#include "../../Render/Backend/SDLRenderer2D.h"

#include <SDL3/SDL.h>

namespace EntityEngine
{

    SDLRenderer2D::SDLRenderer2D(SDL_Renderer *renderer)
        : m_Renderer(renderer) {}

    void SDLRenderer2D::BeginFrame()
    {
        // Por ahora nada especial
    }

    void SDLRenderer2D::EndFrame()
    {
        SDL_RenderPresent(m_Renderer);
    }

    void SDLRenderer2D::Clear(const Color &color)
    {
        SDL_SetRenderDrawColor(m_Renderer, color.r, color.g, color.b, color.a);
        SDL_RenderClear(m_Renderer);
    }

} // namespace EntityEngine
