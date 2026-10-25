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

        using allocator_type = std::pmr::polymorphic_allocator<>;

        VulkanPass() noexcept = default;
        explicit VulkanPass(const allocator_type& allocator) noexcept;
        VulkanPass(const VulkanPass&) = delete;
        VulkanPass(VulkanPass&& other) noexcept;
        VulkanPass(VulkanPass&& other, const allocator_type& allocator);
        ~VulkanPass() override;

        [[nodiscard]] bool valid() const noexcept override;

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return color_attachments_.get_allocator();
        }

        VulkanPass& operator=(const VulkanPass&) = delete;
        VulkanPass& operator=(VulkanPass&& other) noexcept;

    private:
        const VulkanContext* context_ {};
        const VulkanLogicalDevice* logical_device_ {};

        VkRenderPass handle_ {};

        core::Vector<VkAttachmentDescription> color_attachments_;
        core::Vector<VkAttachmentReference> attachments_refs_;

        void destroy() noexcept override;
        [[nodiscard]] std::error_code create(const PassCreateInfo& create_info) noexcept override;
    };
}
