#pragma once

#include "../IRenderer2D.h"

struct SDL_Renderer;

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
        /// Crea el wrapper a partir de un renderer ya inicializado.
        SDLRenderer2D(SDL_Renderer *renderer);

        void BeginFrame() override;
        void EndFrame() override;
        void Clear(const Color &color) override;

    private:
        SDL_Renderer *m_Renderer; ///< Renderer proporcionado por SDL_Window.
    };

} // namespace EntityEngine
