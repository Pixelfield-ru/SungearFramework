//
// Created by stuka on 10.09.2026.
//

#pragma once

#include "sgf/rhi/physical_device.h"

#include <optional>
#include <array>
#include <utility>

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

        explicit VulkanPhysicalDevice(const allocator_type& allocator = {});
        VulkanPhysicalDevice(const VulkanPhysicalDevice&) = delete;
        VulkanPhysicalDevice(VulkanPhysicalDevice&& other) noexcept;
        VulkanPhysicalDevice(VulkanPhysicalDevice&& other, const allocator_type& allocator);

        [[nodiscard]] bool valid() const noexcept override;

        [[nodiscard]] std::expected<SurfaceCapabilities, std::error_code> querySurfaceCapabilities(
            const ISurface& surface, const allocator_type& alloc) const override;

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return IPhysicalDevice::get_allocator();
        }

        VulkanPhysicalDevice& operator=(const VulkanPhysicalDevice&) = delete;
        VulkanPhysicalDevice& operator=(VulkanPhysicalDevice&& other) noexcept;

    private:
        // destroys with VkInstance
        VkPhysicalDevice handle_ {};

        VkPhysicalDeviceFeatures features_ {};

        std::array<std::optional<std::uint32_t>, std::to_underlying(QueueFamilyType::kCount)> queue_families_indices_ {};

        [[nodiscard]] std::expected<SurfaceCapabilities, std::error_code> querySurfaceCapabilities(
            VkSurfaceKHR surface, const allocator_type& alloc) const;

        [[nodiscard]] std::int32_t findMemoryIndex(VkMemoryRequirements requirements, VkMemoryPropertyFlags properties) const noexcept;
    };
}
