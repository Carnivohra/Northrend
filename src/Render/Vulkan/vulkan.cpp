#include "Render/Vulkan/vulkan.hpp"

#include "Render/Vulkan/implementation.hpp"

#include <memory>

namespace Northrend {

VulkanRenderer::VulkanRenderer(const Window& window)
    : implementation(
          std::make_unique<VulkanRendererImplementation>(window)) {}

VulkanRenderer::~VulkanRenderer() = default;

void VulkanRenderer::frame(const RenderFrame& frame) {
    this->implementation->frame(frame);
}

void VulkanRenderer::suspend() {
    this->implementation->suspend();
}

}
