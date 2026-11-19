#pragma once

#include "Render/Texture/texture.hpp"

#include <volk.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace Northrend {

struct RenderFrame;
class TextureBlendData;
class VulkanCommandContext;
class VulkanDevice;
class VulkanFrame;
class VulkanMaterialStore;
class VulkanMeshStore;
class VulkanMeshPipeline;
class VulkanRenderTarget;
class VulkanTextureStore;
class VulkanTextureBlend;
class Window;

class VulkanSwapchain final {
public:
    VulkanSwapchain(VulkanDevice& device, VulkanMeshStore& meshes, VulkanMaterialStore& materials,
        VulkanTextureStore& textures, VkSurfaceKHR surface, const Window& window, const VkClearColorValue& clearColor);
    ~VulkanSwapchain();

    VulkanSwapchain(const VulkanSwapchain&) = delete;
    VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;

    void render(const RenderFrame& frame);
    void store_texture(TextureHandle handle, const TextureBlendData& data);
    void cancel_texture(TextureHandle handle) noexcept;
    [[nodiscard]] bool texture_ready(TextureHandle handle) const noexcept;
    void suspend() noexcept;
    [[nodiscard]] std::uint64_t retirement_serial() const noexcept;

private:
    static constexpr std::size_t FramesInFlight = 2;

    [[nodiscard]] static VkSurfaceFormatKHR choose_surface_format(const std::vector<VkSurfaceFormatKHR>& formats);
    [[nodiscard]] static VkExtent2D choose_surface_extent(
        const VkSurfaceCapabilitiesKHR& capabilities, int width, int height);
    [[nodiscard]] static VkPresentModeKHR choose_present_mode(const std::vector<VkPresentModeKHR>& presentModes);
    [[nodiscard]] static VkCompositeAlphaFlagBitsKHR choose_composite_alpha(VkCompositeAlphaFlagsKHR supported);

    void create_frame_resources();
    void create_swapchain_resources();
    void recreate();
    void collect_retired(std::uint64_t completedSerial) noexcept;
    void destroy_swapchain_resources() noexcept;
    void destroy() noexcept;

    VulkanDevice& device;
    VulkanMeshStore& meshes;
    VulkanMaterialStore& materials;
    VulkanTextureStore& textures;
    std::unique_ptr<VulkanTextureBlend> textureBlend;
    VkSurfaceKHR surface;
    const Window& window;
    VkClearColorValue clearColor;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat imageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D extent {};
    VkSurfaceTransformFlagBitsKHR surfaceTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    int pixelWidth = 0;
    int pixelHeight = 0;
    std::vector<VkImage> images;
    std::vector<std::unique_ptr<VulkanFrame>> frames;
    std::array<std::uint64_t, FramesInFlight> frameSerials {};
    std::vector<VkFence> imagesInFlight;
    std::unique_ptr<VulkanRenderTarget> target;
    std::unique_ptr<VulkanMeshPipeline> pipeline;
    std::unique_ptr<VulkanCommandContext> commands;
    std::size_t currentFrame = 0;
    std::uint64_t submittedSerial = 0;
    std::uint64_t completedSerial = 0;
};

}
