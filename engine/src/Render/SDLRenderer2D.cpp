#include "../../include/Render/Backend/SDLRenderer2D.h"
#include "../../include/Core/log.h"

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

    TextureHandle SDLRenderer2D::CreateTexture(const void *data, int width, int height)
    {

        if (!data || width <= 0 || height <= 0)
        {
            EE_LOG_ERROR("Invalid texture data or dimensions.");
            return 0;
        }

        SDL_Texture* sdlTexture = SDL_CreateTexture(m_Renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, width, height);
        if (!sdlTexture)
        {
            EE_LOG_ERROR(std::string("Error creando textura: ") + SDL_GetError());
            return 0;
        }

        if (!SDL_UpdateTexture(sdlTexture, nullptr, data, width * 4)) // Asumiendo RGBA8888
        {
            EE_LOG_ERROR(std::string("Error actualizando textura: ") + SDL_GetError());
            SDL_DestroyTexture(sdlTexture);
            return 0;
        }

        TextureHandle handle = m_NextTextureHandle++;
        m_Textures[handle] = sdlTexture;

        return handle;
    }

    void SDLRenderer2D::DestroyTexture(TextureHandle texture)
    {
        if (texture == 0)
        return;

        auto it = m_Textures.find(texture);
        if (it == m_Textures.end())
        {
            EE_LOG_ERROR("Intento de destruir una textura inexistente.");
            return;
        }

        SDL_DestroyTexture(it->second);
        m_Textures.erase(it);
    }

    void SDLRenderer2D::DrawTexture(TextureHandle texture, const Rect *srcRect, const Rect *destRect, float rotation, const Color& tint)
    {

        if (texture == 0)
            return;

        auto it = m_Textures.find(texture);
        if (it == m_Textures.end())
        {
            EE_LOG_ERROR("Intento de dibujar una textura inexistente.");
            return;
        }

        SDL_Texture* sdlTexture = it->second;
        
        if (srcRect && destRect)
        {
            SDL_FRect sdlDestRect{destRect->x, destRect->y, destRect->width, destRect->height};
            SDL_FRect sdlSrcRect{(srcRect->x), (srcRect->y), (srcRect->width), (srcRect->height)};

            SDL_SetTextureColorMod(sdlTexture, tint.r, tint.g, tint.b);
            SDL_SetTextureAlphaMod(sdlTexture, tint.a);
            SDL_RenderTextureRotated(m_Renderer, sdlTexture, &sdlSrcRect, &sdlDestRect, rotation, nullptr, SDL_FLIP_NONE);
        }
        else if (destRect)
        {
            SDL_FRect sdlDestRect{destRect->x, destRect->y, destRect->width, destRect->height};

            SDL_SetTextureColorMod(sdlTexture, tint.r, tint.g, tint.b);
            SDL_SetTextureAlphaMod(sdlTexture, tint.a);
            SDL_RenderTextureRotated(m_Renderer, sdlTexture, nullptr, &sdlDestRect, rotation, nullptr, SDL_FLIP_NONE);
        }
        else
        {
            SDL_SetTextureColorMod(sdlTexture, tint.r, tint.g, tint.b);
            SDL_SetTextureAlphaMod(sdlTexture, tint.a);
            SDL_RenderTextureRotated(m_Renderer, sdlTexture, nullptr, nullptr, rotation, nullptr, SDL_FLIP_NONE);
        }

    }

} // namespace EntityEngine
