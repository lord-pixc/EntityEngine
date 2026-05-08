#include "../../include/Render/Backend/SDLRenderer2D.h"

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

    void SDLRenderer2D::DrawRect(const Rect &rect, const Color &color, bool filled)
    {
        SDL_SetRenderDrawColor(m_Renderer, color.r, color.g, color.b, color.a);
        SDL_FRect sdlRect{static_cast<float>(rect.x), static_cast<float>(rect.y),
                          static_cast<float>(rect.width), static_cast<float>(rect.height)};
        if (filled)
        {
            SDL_RenderFillRect(m_Renderer, &sdlRect);
        }
        else
        {
            SDL_RenderRect(m_Renderer, &sdlRect);
        }
    }

    void SDLRenderer2D::DrawLine(const Line &line, const Color &color)
    {
        SDL_SetRenderDrawColor(m_Renderer, color.r, color.g, color.b, color.a);
        SDL_RenderLine(m_Renderer, static_cast<float>(line.x1), static_cast<float>(line.y1),
                         static_cast<float>(line.x2), static_cast<float>(line.y2));
    }

    void SDLRenderer2D::DrawCircle(const Circle &circle, const Color &color, bool filled)
    {
        SDL_SetRenderDrawColor(m_Renderer, color.r, color.g, color.b, color.a);

        // Algoritmo de Bresenham para círculos
        int x0 = static_cast<int>(circle.centerX);
        int y0 = static_cast<int>(circle.centerY);
        int radius = static_cast<int>(circle.radius);

        int x = radius;
        int y = 0;
        int err = 0;

        while (x >= y)
        {
            if (filled)
            {
                SDL_RenderLine(m_Renderer, x0 - x, y0 + y, x0 + x, y0 + y);
                SDL_RenderLine(m_Renderer, x0 - y, y0 + x, x0 + y, y0 + x);
                SDL_RenderLine(m_Renderer, x0 - x, y0 - y, x0 + x, y0 - y);
                SDL_RenderLine(m_Renderer, x0 - y, y0 - x, x0 + y, y0 - x);
            }
            else
            {
                SDL_RenderPoint(m_Renderer, x0 + x, y0 + y);
                SDL_RenderPoint(m_Renderer, x0 + y, y0 + x);
                SDL_RenderPoint(m_Renderer, x0 - y, y0 + x);
                SDL_RenderPoint(m_Renderer, x0 - x, y0 + y);
                SDL_RenderPoint(m_Renderer, x0 - x, y0 - y);
                SDL_RenderPoint(m_Renderer, x0 - y, y0 - x);
                SDL_RenderPoint(m_Renderer, x0 + y, y0 - x);
                SDL_RenderPoint(m_Renderer, x0 + x, y0 - y);
            }

            if (err <= 0)
            {
                err += 2 * ++y + 1;
            }
            if (err > 0)
            {
                err -= 2 * --x + 1;
            }
        }
    }

} // namespace EntityEngine
