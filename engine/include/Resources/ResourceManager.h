#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include "../Render/IRenderer2D.h"

namespace EntityEngine
{

    class ResourceManager
    {
    public:

    TextureHandle Load(const std::string& filePath);
    void Unload(const std::string& filePath);

    private:
        IRenderer2D* m_Renderer; // Pointer to the renderer, needed for texture creation
        std::unordered_map<std::string, std::shared_ptr<void>> m_Resources;
    };

} // namespace EntityEngine