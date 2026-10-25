#include "Render/Vulkan/vulkan.hpp"

#include "Backend/Window/window.hpp"
#include "Render/Vulkan/Platform/surface.hpp"
#include "Render/Vulkan/device.hpp"
#include "Render/Vulkan/mesh_store.hpp"
#include "Render/Vulkan/swapchain.hpp"
#include "Render/Vulkan/triangle.hpp"

#include <volk.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace Northrend::VulkanDetail {

constexpr const char* ValidationLayer = "VK_LAYER_KHRONOS_validation";

bool has_instance_extension(const char* name) {
    std::uint32_t count = 0;

    if (vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr) != VK_SUCCESS) {
        return false;
    }

    std::vector<VkExtensionProperties> extensions(count);

    if (vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data()) != VK_SUCCESS) {
        return false;
    }

    return std::ranges::any_of(extensions, [name](const VkExtensionProperties& extension) {
        return std::strcmp(extension.extensionName, name) == 0;
    });
}

bool has_instance_layer(const char* name) {
    std::uint32_t count = 0;

    if (vkEnumerateInstanceLayerProperties(&count, nullptr) != VK_SUCCESS) {
        return false;
    }

    std::vector<VkLayerProperties> layers(count);

    if (vkEnumerateInstanceLayerProperties(&count, layers.data()) != VK_SUCCESS) {
        return false;
    }

    return std::ranges::any_of(layers, [name](const VkLayerProperties& layer) {
        return std::strcmp(layer.layerName, name) == 0;
    });
}

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void*) {
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << "Vulkan: " << callbackData->pMessage << '\n';
    }

    return VK_FALSE;
}

VkDebugUtilsMessengerCreateInfoEXT debug_messenger_info() {
    return VkDebugUtilsMessengerCreateInfoEXT{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = nullptr,
        .flags = 0,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debug_callback,
        .pUserData = nullptr
    };
}

}

namespace Northrend {

class VulkanRenderer::Implementation final {
public:
    explicit Implementation(const Window& window) {
        const VkResult initializationResult = volkInitialize();

        if (initializationResult != VK_SUCCESS) {
            throw std::runtime_error(
                "Failed to initialize the Vulkan loader: " +
                std::to_string(initializationResult));
        }

        const NativeWindowHandle nativeHandle = window.native_handle();
        const char* platformExtension = VulkanPlatform::surface_extension(nativeHandle);

        if (!VulkanDetail::has_instance_extension(VK_KHR_SURFACE_EXTENSION_NAME)) {
            throw std::runtime_error("Vulkan does not support the required surface extension");
        }

        if (!VulkanDetail::has_instance_extension(platformExtension)) {
            throw std::runtime_error(
                std::string("Vulkan does not support the required platform extension: ") +
                platformExtension);
        }

        std::vector<const char*> extensions{
            VK_KHR_SURFACE_EXTENSION_NAME,
            platformExtension
        };
        std::vector<const char*> layers;

#if defined(NORTHREND_VULKAN_VALIDATION)
        this->validationEnabled =
            VulkanDetail::has_instance_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME) &&
            VulkanDetail::has_instance_layer(VulkanDetail::ValidationLayer);

        if (this->validationEnabled) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            layers.push_back(VulkanDetail::ValidationLayer);
        }
#endif

        const std::uint32_t apiVersion = std::min(
            volkGetInstanceVersion(),
            static_cast<std::uint32_t>(VK_API_VERSION_1_3));
        const VkApplicationInfo applicationInfo{
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .pApplicationName = "Northrend",
            .applicationVersion = VK_MAKE_API_VERSION(0, 0, 0, 1),
            .pEngineName = "Northrend",
            .engineVersion = VK_MAKE_API_VERSION(0, 0, 0, 1),
            .apiVersion = apiVersion
        };
        VkDebugUtilsMessengerCreateInfoEXT debugInfo = VulkanDetail::debug_messenger_info();
        const VkInstanceCreateInfo instanceInfo{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = this->validationEnabled ? &debugInfo : nullptr,
            .flags = 0,
            .pApplicationInfo = &applicationInfo,
            .enabledLayerCount = static_cast<std::uint32_t>(layers.size()),
            .ppEnabledLayerNames = layers.data(),
            .enabledExtensionCount = static_cast<std::uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data()
        };

        const VkResult instanceResult = vkCreateInstance(&instanceInfo, nullptr, &this->instance);

        if (instanceResult != VK_SUCCESS) {
            throw std::runtime_error(
                "Failed to create the Vulkan instance: " + std::to_string(instanceResult));
        }

        volkLoadInstance(this->instance);

        try {
            if (this->validationEnabled) {
                const VkResult debugResult = vkCreateDebugUtilsMessengerEXT(
                    this->instance,
                    &debugInfo,
                    nullptr,
                    &this->debugMessenger);

                if (debugResult != VK_SUCCESS) {
                    throw std::runtime_error(
                        "Failed to create the Vulkan debug messenger: " +
                        std::to_string(debugResult));
                }
            }

            this->surface = VulkanPlatform::create_surface(this->instance, nativeHandle);
            this->device = std::make_unique<VulkanDevice>(this->instance, this->surface);
            this->meshes = std::make_unique<VulkanMeshStore>(*this->device);
            static_cast<void>(VulkanTriangle::create(*this->meshes));
            this->swapchain = std::make_unique<VulkanSwapchain>(
                *this->device,
                *this->meshes,
                this->surface,
                window,
                ClearColor);
            this->status = RendererStatus::Ready;
        } catch (...) {
            this->destroy();
            throw;
        }
    }

    ~Implementation() {
        this->destroy();
    }

    RendererStatus frame(const RenderFrame& frame) {
        this->swapchain->render(frame);
        return this->status;
    }

    void suspend() noexcept {
        if (this->swapchain) {
            this->swapchain->suspend();
        }
    }

private:
    static constexpr VkClearColorValue ClearColor{
        .float32 = { 0.015F, 0.035F, 0.080F, 1.000F }
    };

    void destroy() noexcept {
        if (this->device) {
            this->device->wait_idle();
        }

        this->swapchain.reset();
        this->meshes.reset();
        this->device.reset();

        if (this->surface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(this->instance, this->surface, nullptr);
            this->surface = VK_NULL_HANDLE;
        }

        if (this->debugMessenger != VK_NULL_HANDLE) {
            vkDestroyDebugUtilsMessengerEXT(this->instance, this->debugMessenger, nullptr);
            this->debugMessenger = VK_NULL_HANDLE;
        }

        if (this->instance != VK_NULL_HANDLE) {
            vkDestroyInstance(this->instance, nullptr);
            this->instance = VK_NULL_HANDLE;
        }

        this->status = RendererStatus::Uninitialized;
    }

    RendererStatus status = RendererStatus::Initializing;
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    std::unique_ptr<VulkanDevice> device;
    std::unique_ptr<VulkanMeshStore> meshes;
    std::unique_ptr<VulkanSwapchain> swapchain;
    bool validationEnabled = false;
};

VulkanRenderer::VulkanRenderer(const Window& window)
    : implementation(std::make_unique<Implementation>(window)) {}

VulkanRenderer::~VulkanRenderer() = default;

RendererStatus VulkanRenderer::frame(const RenderFrame& frame) {
    return this->implementation->frame(frame);
}

void VulkanRenderer::suspend() {
    this->implementation->suspend();
}

}
