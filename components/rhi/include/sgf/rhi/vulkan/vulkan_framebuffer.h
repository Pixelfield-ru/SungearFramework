//
// Created by stuka on 20.09.2026.
//

#pragma once

#include <system_error>
#include <vulkan/vulkan_core.h>

#include "sgf/rhi/framebuffer.h"

namespace sgf::rhi
{
    struct VulkanContext;

    struct VulkanFramebuffer final : IFramebuffer
    {
        friend struct VulkanLogicalDevice;
        friend struct VulkanCommandBuffer;

        VulkanFramebuffer() noexcept = default;
        VulkanFramebuffer(const VulkanFramebuffer&) = delete;
        VulkanFramebuffer(VulkanFramebuffer&& other) noexcept;
        ~VulkanFramebuffer() override;

        VulkanFramebuffer& operator=(const VulkanFramebuffer&) = delete;
        VulkanFramebuffer& operator=(VulkanFramebuffer&& other) noexcept;

    private:
        const VulkanContext* context_ {};
        const VulkanLogicalDevice* logical_device_ {};

        VkFramebuffer handle_ {};

        std::pmr::memory_resource* memory_resource_ {};

        void destroy() noexcept override;
        [[nodiscard]] std::error_code create(const FramebufferCreateInfo& create_info) noexcept override;
    };
}
