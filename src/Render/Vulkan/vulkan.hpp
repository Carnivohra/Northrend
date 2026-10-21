#pragma once

#include "Render/renderer.hpp"

#include <memory>

namespace Northrend {

class Window;

class VulkanRenderer final : public Renderer {
public:
    explicit VulkanRenderer(const Window& window);
    ~VulkanRenderer() override;

    VulkanRenderer(const VulkanRenderer&) = delete;
    VulkanRenderer& operator=(const VulkanRenderer&) = delete;

    RendererStatus frame() override;
    void suspend() override;

private:
    class Implementation;

    std::unique_ptr<Implementation> implementation;
};

}
