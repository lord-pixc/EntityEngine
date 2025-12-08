#pragma once

#include <cstdint>

namespace EntityEngine
{
    struct Color
    {
        std::uint8_t r, g, b, a;
    };

    class IRenderer2D
    {
    public:
        virtual ~IRenderer2D() = default;

        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;

        virtual void Clear(const Color &color) = 0;
    };
} // namespace EntityEngine