//
// Created by stuka on 10.09.2026.
//

#pragma once

#include "sgf/rhi/physical_device.h"

#include <optional>
#include <array>
#include <utility>
#include <vector>

#include <vulkan/vulkan.h>

namespace sgf::rhi
{
    struct VulkanPhysicalDevice final : IPhysicalDevice
    {
        friend struct VulkanContext;
        friend struct VulkanLogicalDevice;
        friend struct VulkanSwapchain;
        friend struct VulkanBuffer;
        friend struct VulkanImage;

        VulkanPhysicalDevice() = default;
        VulkanPhysicalDevice(const VulkanPhysicalDevice&) = delete;
        VulkanPhysicalDevice(VulkanPhysicalDevice&& other) noexcept;

        [[nodiscard]] bool valid() const noexcept override;

        VulkanPhysicalDevice& operator=(const VulkanPhysicalDevice&) = delete;
        VulkanPhysicalDevice& operator=(VulkanPhysicalDevice&& other) noexcept;

    private:
        struct SurfaceSupport
        {
            core::Vector<VkSurfaceFormatKHR> formats_;
            core::Vector<VkPresentModeKHR> present_modes_;
        };

        // destroys with VkInstance
        VkPhysicalDevice handle_ {};

        VkPhysicalDeviceFeatures features_ {};
        SurfaceSupport surface_support_;

        std::array<std::optional<std::uint32_t>, std::to_underlying(QueueFamilyType::kCount)> queue_families_indices_ {};

        [[nodiscard]] std::int32_t findMemoryIndex(VkMemoryRequirements requirements, VkMemoryPropertyFlags properties) const noexcept;
    };
}
