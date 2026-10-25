//
// Created by stuka on 10.09.2026.
//

#pragma once

#include <vector>
#include <string>

#define GLFW_INCLUDE_VULKAN
#include <memory>
#include <GLFW/glfw3.h>

#include "sgf/rhi/context.h"
#include "sgf/core/memory.h"

namespace sgf::rhi
{
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
        VkDebugUtilsMessageTypeFlagsEXT message_type,
        const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
        void* p_user_data);

    struct VulkanContextCreateInfo final : ContextCreateInfo
    {
        /// Optional.
        core::Vector<core::String> validation_layers;
        /// Optional.
        core::Vector<core::String> extensions {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
        /// Optional.
        bool enable_validation_layers {};
        /// Optional. If nullptr then pf::rhi::debugCallback is used.
        VkBool32 (*api_debug_output_func)(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                         VkDebugUtilsMessageTypeFlagsEXT message_type,
                                         const VkDebugUtilsMessengerCallbackDataEXT*
                                         p_callback_data,
                                         void* p_user_data) { };
        /// Optional. Defaults to new/delete-backed host allocator.
        core::UniquePtr<VkAllocationCallbacks> allocator;

        VulkanContextCreateInfo() = default;
        explicit VulkanContextCreateInfo(const allocator_type& allocator) :
            ContextCreateInfo(allocator),
            validation_layers(allocator),
            extensions(allocator)
        {
            extensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        }
        VulkanContextCreateInfo(const VulkanContextCreateInfo&) = delete;
        VulkanContextCreateInfo(VulkanContextCreateInfo&&) noexcept = default;
        VulkanContextCreateInfo(VulkanContextCreateInfo&& other, const allocator_type& allocator) :
            ContextCreateInfo(std::move(other), allocator),
            validation_layers(std::move(other.validation_layers), allocator),
            extensions(std::move(other.extensions), allocator),
            enable_validation_layers(other.enable_validation_layers),
            api_debug_output_func(other.api_debug_output_func),
            allocator(std::move(other.allocator))
        { }

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return extensions.get_allocator();
        }

        VulkanContextCreateInfo& operator=(const VulkanContextCreateInfo&) = delete;
        VulkanContextCreateInfo& operator=(VulkanContextCreateInfo&&) noexcept = default;
    };

    struct VulkanContext final : IContext
    {
        friend struct VulkanLogicalDevice;
        friend struct VulkanSwapchain;
        friend struct VulkanImage;
        friend struct VulkanImageView;
        friend struct VulkanSampler;
        friend struct VulkanShader;
        friend struct VulkanPipelineState;
        friend struct VulkanFramebuffer;
        friend struct VulkanPass;
        friend struct VulkanBuffer;
        friend struct VulkanDescriptorSet;

        VulkanContext() noexcept = default;
        explicit VulkanContext(const allocator_type& allocator) noexcept;
        VulkanContext(const VulkanContext&) = delete;
        VulkanContext(VulkanContext&& other) noexcept;
        VulkanContext(VulkanContext&& other, const allocator_type& allocator);
        ~VulkanContext() noexcept override;

        [[nodiscard]] bool valid() const noexcept override;

        [[nodiscard]] core::Vector<core::SharedPtr<IPhysicalDevice>> physicalDevices() noexcept override;

        [[nodiscard]] std::expected<core::UniquePtr<ILogicalDevice>, std::error_code> createLogicalDevice(
            const LogicalDeviceCreateInfo& create_info, const allocator_type& alloc) noexcept override;

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return enabled_extensions_.get_allocator();
        }

        VulkanContext& operator=(const VulkanContext&) = delete;
        VulkanContext& operator=(VulkanContext&& other) noexcept;

        [[nodiscard]] static std::error_code create(VulkanContextCreateInfo create_info, VulkanContext& output) noexcept;

    private:
        PFN_vkCreateDebugUtilsMessengerEXT create_messenger_ {};
        PFN_vkDestroyDebugUtilsMessengerEXT destroy_messenger_ {};

        core::UniquePtr<VkAllocationCallbacks> allocator_;

        VkInstance instance_ {};
        VkSurfaceKHR surface_ {};
        VkDebugUtilsMessengerEXT messenger_ {};

        core::Vector<core::String> enabled_validation_layers_;
        core::Vector<core::String> enabled_extensions_;

        void destroy() noexcept;
    };
}
