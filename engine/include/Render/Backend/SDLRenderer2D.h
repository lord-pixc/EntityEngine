#pragma once

#include "../IRenderer2D.h"
#include <unordered_map>

struct SDL_Renderer;
struct SDL_Texture;

namespace EntityEngine
{

    /**
     * @brief Implementación de IRenderer2D usando el backend de SDL.
     *
     * Opera sobre un SDL_Renderer externo, por lo que la clase no es dueña
     * del recurso y asume que su ciclo de vida se gestiona desde Window.
     */
    class SDLRenderer2D : public IRenderer2D
    {
    public:
        struct texture2D
        {
            SDL_Texture* texture;
            int width;
            int height;

            int refcount;
        };

        /// Crea el wrapper a partir de un renderer ya inicializado.
        SDLRenderer2D(SDL_Renderer *renderer);

        void BeginFrame() override;
        void EndFrame() override;
        void Clear(const Color &color) override;

        void DrawRect(const Rect &rect, const Color &color, bool filled = true) override;
        void DrawLine(const Line &line, const Color &color) override;
        void DrawCircle(const Circle &circle, const Color &color, bool filled = true) override;
        void DrawTexture(TextureHandle texture, const Rect *srcRect = nullptr, const Rect *destRect = nullptr, float rotation = 0.0f, const Color& tint = {255, 255, 255, 255}) override;
        TextureHandle CreateTexture(const void *data, int width, int height) override;
        void DestroyTexture(TextureHandle texture) override;


    private:
        SDL_Renderer *m_Renderer; ///< Renderer proporcionado por SDL_Window.
        std::unordered_map<TextureHandle, SDL_Texture*> m_Textures;
        TextureHandle m_NextTextureHandle = 1;
    };

} // namespace EntityEngine
