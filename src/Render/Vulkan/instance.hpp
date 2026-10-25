#pragma once

#include <volk.h>

namespace Northrend {

class VulkanInstance final {
public:
    explicit VulkanInstance(const char* platformExtension);
    ~VulkanInstance();

    VulkanInstance(const VulkanInstance&) = delete;
    VulkanInstance& operator=(const VulkanInstance&) = delete;

    [[nodiscard]] VkInstance handle() const noexcept;

private:
    static constexpr const char* ValidationLayer =
        "VK_LAYER_KHRONOS_validation";

    [[nodiscard]] static bool has_extension(const char* name);
    [[nodiscard]] static bool has_layer(const char* name);
    [[nodiscard]] static VkDebugUtilsMessengerCreateInfoEXT debug_info() noexcept;
    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type,
        const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
        void* userData);

    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
};

}
