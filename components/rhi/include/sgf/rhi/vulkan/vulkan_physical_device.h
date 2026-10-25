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
        explicit VulkanPhysicalDevice(const allocator_type& allocator);
        VulkanPhysicalDevice(const VulkanPhysicalDevice&) = delete;
        VulkanPhysicalDevice(VulkanPhysicalDevice&& other) noexcept;
        VulkanPhysicalDevice(VulkanPhysicalDevice&& other, const allocator_type& allocator);

        [[nodiscard]] bool valid() const noexcept override;

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return IPhysicalDevice::get_allocator();
        }

        VulkanPhysicalDevice& operator=(const VulkanPhysicalDevice&) = delete;
        VulkanPhysicalDevice& operator=(VulkanPhysicalDevice&& other) noexcept;

    private:
        struct SurfaceSupport
        {
            using allocator_type = std::pmr::polymorphic_allocator<>;

            core::Vector<VkSurfaceFormatKHR> formats_;
            core::Vector<VkPresentModeKHR> present_modes_;

            SurfaceSupport() = default;
            explicit SurfaceSupport(const allocator_type& allocator) :
                formats_(allocator),
                present_modes_(allocator)
            { }
            SurfaceSupport(const SurfaceSupport&) = default;
            SurfaceSupport(const SurfaceSupport& other, const allocator_type& allocator) :
                formats_(other.formats_, allocator),
                present_modes_(other.present_modes_, allocator)
            { }
            SurfaceSupport(SurfaceSupport&&) noexcept = default;
            SurfaceSupport(SurfaceSupport&& other, const allocator_type& allocator) :
                formats_(std::move(other.formats_), allocator),
                present_modes_(std::move(other.present_modes_), allocator)
            { }

            [[nodiscard]] allocator_type get_allocator() const noexcept
            {
                return formats_.get_allocator();
            }

            SurfaceSupport& operator=(const SurfaceSupport&) = default;
            SurfaceSupport& operator=(SurfaceSupport&&) noexcept = default;
        };

        // destroys with VkInstance
        VkPhysicalDevice handle_ {};

        VkPhysicalDeviceFeatures features_ {};
        SurfaceSupport surface_support_;

        std::array<std::optional<std::uint32_t>, std::to_underlying(QueueFamilyType::kCount)> queue_families_indices_ {};

        [[nodiscard]] std::int32_t findMemoryIndex(VkMemoryRequirements requirements, VkMemoryPropertyFlags properties) const noexcept;
    };
}
