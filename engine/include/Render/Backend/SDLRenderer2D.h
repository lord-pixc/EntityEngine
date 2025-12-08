#pragma once

#include "../IRenderer2D.h"

struct SDL_Renderer;

namespace EntityEngine
{

    class SDLRenderer2D : public IRenderer2D
    {
    public:
        SDLRenderer2D(SDL_Renderer *renderer);

        void BeginFrame() override;
        void EndFrame() override;
        void Clear(const Color &color) override;

    private:
        SDL_Renderer *m_Renderer;
    };

} // namespace EntityEngine
