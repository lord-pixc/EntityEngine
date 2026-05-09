#pragma once

#include <cstdint>

namespace EntityEngine
{
    struct Color
    {
        std::uint8_t r, g, b, a;
    };

    struct Rect
    {
        float x, y, width, height;
    };

    struct Circle
    {
        float centerX, centerY, radius;
    };

    struct Line
    {
        float x1, y1, x2, y2;
    };

    using TextureHandle = std::uint32_t;

    /**
     * @brief Interfaz genérica para renderizadores 2D.
     *
     * Las implementaciones concretas (SDL, OpenGL, etc.) deben encargarse de
     * preparar el frame, limpiarlo con un color y finalizarlo presentando el
     * resultado en pantalla.
     */
    class IRenderer2D
    {
    public:
        virtual ~IRenderer2D() = default;

        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;

        virtual void Clear(const Color &color) = 0;

        virtual void DrawRect(const Rect &rect, const Color &color, bool filled = true) = 0;
        virtual void DrawLine(const Line &line, const Color &color) = 0;
        virtual void DrawCircle(const Circle &circle, const Color &color, bool filled = false) = 0;
        virtual void DrawTexture(TextureHandle texture, const Rect *srcRect = nullptr, const Rect *destRect = nullptr, float rotation = 0.0f, const Color& tint = {255, 255, 255, 255}) = 0;
        virtual TextureHandle CreateTexture(const void *data, int width, int height) = 0;
        virtual void DestroyTexture(TextureHandle texture) = 0;
    };
} // namespace EntityEngine
