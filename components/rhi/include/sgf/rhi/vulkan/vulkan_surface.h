//
// Created by stuka on 17.11.2026.
//

#pragma once

#include <vulkan/vulkan_core.h>

#include "sgf/rhi/surface.h"

namespace sgf::rhi
{
    struct VulkanContext;

    struct VulkanSurface final : ISurface
    {
        VulkanSurface() noexcept = default;
        VulkanSurface(const VulkanSurface&) = delete;
        VulkanSurface(VulkanSurface&& other) noexcept;
        ~VulkanSurface() noexcept override;

        VulkanSurface& operator=(const VulkanSurface&) = delete;
        VulkanSurface& operator=(VulkanSurface&& other) noexcept;

    private:
        const VulkanContext* context_ {};

        VkSurfaceKHR handle_ {};

        void destroy() override;
        [[nodiscard]] std::error_code create(const SurfaceCreateInfo& create_info) override;
    };
}