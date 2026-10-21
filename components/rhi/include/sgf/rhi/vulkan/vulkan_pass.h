//
// Created by stuka on 29.09.2026.
//

#pragma once

#include <vulkan/vulkan_core.h>

#include "sgf/rhi/pass.h"

namespace sgf::rhi
{
    struct VulkanContext;

    struct VulkanPass final : IPass
    {
        friend struct VulkanPipelineState;
        friend struct VulkanFramebuffer;
        friend struct VulkanLogicalDevice;
        friend struct VulkanCommandBuffer;

        VulkanPass() noexcept = default;
        VulkanPass(const VulkanPass&) = delete;
        VulkanPass(VulkanPass&& other) noexcept;
        ~VulkanPass() override;

        [[nodiscard]] bool valid() const noexcept override;

        VulkanPass& operator=(const VulkanPass&) = delete;
        VulkanPass& operator=(VulkanPass&& other) noexcept;

    private:
        const VulkanContext* context_ {};
        const VulkanLogicalDevice* logical_device_ {};

        VkRenderPass handle_ {};

        core::Vector<VkAttachmentDescription> color_attachments_;
        core::Vector<VkAttachmentReference> attachments_refs_;

        std::pmr::memory_resource* memory_resource_ {};

        void destroy() noexcept override;
        [[nodiscard]] std::error_code create(const PassCreateInfo& create_info) noexcept override;
    };
}
