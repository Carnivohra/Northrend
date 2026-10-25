#pragma once

#include "Render/renderer.hpp"

#include <memory>

namespace Northrend {

class Window;
class VulkanRendererImplementation;

class VulkanRenderer final : public Renderer {
public:
    explicit VulkanRenderer(const Window& window);
    ~VulkanRenderer() override;

    VulkanRenderer(const VulkanRenderer&) = delete;
    VulkanRenderer& operator=(const VulkanRenderer&) = delete;

    void frame(const RenderFrame& frame) override;
    void suspend() override;

private:
    std::unique_ptr<VulkanRendererImplementation> implementation;
};

}
