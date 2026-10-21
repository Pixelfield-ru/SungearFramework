//
// Created by stuka on 20.09.2026.
//

#include "sgf/rhi/vulkan/vulkan_framebuffer.h"

#include "sgf/core/errors.h"
#include "sgf/rhi/vulkan/vulkan_context.h"
#include "sgf/rhi/vulkan/vulkan_image_view.h"
#include "sgf/rhi/vulkan/vulkan_logical_device.h"
#include "sgf/rhi/vulkan/vulkan_pass.h"
#include "sgf/rhi/vulkan/vulkan_pipeline_state.h"

sgf::rhi::VulkanFramebuffer::VulkanFramebuffer(VulkanFramebuffer&& other) noexcept :
    context_(std::exchange(other.context_, nullptr)),
    logical_device_(std::exchange(other.logical_device_, nullptr)),
    handle_(std::exchange(other.handle_, VK_NULL_HANDLE)),
    memory_resource_(std::exchange(other.memory_resource_, VK_NULL_HANDLE))
{

}

sgf::rhi::VulkanFramebuffer::~VulkanFramebuffer()
{
    destroy();
}

sgf::rhi::VulkanFramebuffer& sgf::rhi::VulkanFramebuffer::operator=(VulkanFramebuffer&& other) noexcept
{
    if(this == std::addressof(other)) return *this;

    destroy();

    context_ = std::exchange(other.context_, nullptr);
    logical_device_ = std::exchange(other.logical_device_, nullptr);
    handle_ = std::exchange(other.handle_, VK_NULL_HANDLE);
    memory_resource_ = std::exchange(other.memory_resource_, {});

    return *this;
}

void sgf::rhi::VulkanFramebuffer::destroy() noexcept
{
    if(handle_)
    {
        vkDestroyFramebuffer(logical_device_->handle_, handle_, context_->allocator_.get());
        handle_ = VK_NULL_HANDLE;
    }
}

std::error_code sgf::rhi::VulkanFramebuffer::create(const FramebufferCreateInfo& create_info) noexcept
{
    if(!create_info.memory_resource) return core::Error::kInvalidArgument;

    const auto* vulkan_context = static_cast<const VulkanContext*>(create_info.context);
    const auto* vulkan_logical_device = static_cast<const VulkanLogicalDevice*>(create_info.logical_device);
    const auto* vulkan_pass = static_cast<const VulkanPass*>(create_info.pass);

    context_ = vulkan_context;
    logical_device_ = vulkan_logical_device;
    memory_resource_ = create_info.memory_resource;

    core::Vector<VkImageView> vulkan_attachments(create_info.images.size(), memory_resource_);
    for(size_t i = 0; i < create_info.images.size(); ++i)
    {
        const auto* vulkan_image_view = static_cast<const VulkanImageView*>(create_info.images[i]);

        vulkan_attachments[i] = vulkan_image_view->handle_;
    }

    VkFramebufferCreateInfo framebuffer_info{};
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.renderPass = vulkan_pass->handle_;
    framebuffer_info.attachmentCount = vulkan_attachments.size();
    framebuffer_info.pAttachments = vulkan_attachments.data();
    framebuffer_info.width = create_info.size.x;
    framebuffer_info.height = create_info.size.y;
    framebuffer_info.layers = 1;

    if(vkCreateFramebuffer(vulkan_logical_device->handle_, &framebuffer_info, vulkan_context->allocator_.get(), &handle_) != VK_SUCCESS)
    {
        return VulkanError::kCannotCreateFramebuffer;
    }

    return {};
}
