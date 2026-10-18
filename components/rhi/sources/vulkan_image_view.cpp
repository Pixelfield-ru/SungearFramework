//
// Created by stuka on 14.09.2026.
//

#include "sgf/rhi/vulkan/vulkan_image_view.h"

#include "sgf/rhi/vulkan/vulkan_logical_device.h"
#include "sgf/rhi/vulkan/vulkan_context.h"
#include "sgf/rhi/vulkan/vulkan_image.h"
#include "sgf/rhi/vulkan/vulkan_types.h"

sgf::rhi::VulkanImageView::VulkanImageView(VulkanImageView&& other) noexcept :
    context_(std::exchange(other.context_, nullptr)),
    logical_device_(std::exchange(other.logical_device_, nullptr)),
    handle_(std::exchange(other.handle_, VK_NULL_HANDLE))
{

}

sgf::rhi::VulkanImageView::~VulkanImageView() noexcept
{
    destroy();
}

bool sgf::rhi::VulkanImageView::valid() const noexcept
{
    return handle_ != nullptr;
}

sgf::rhi::VulkanImageView& sgf::rhi::VulkanImageView::operator=(VulkanImageView&& other) noexcept
{
    if(this == std::addressof(other)) return *this;

    destroy();

    context_ = std::exchange(other.context_, nullptr);
    logical_device_ = std::exchange(other.logical_device_, nullptr);
    handle_ = std::exchange(other.handle_, VK_NULL_HANDLE);

    return *this;
}

void sgf::rhi::VulkanImageView::destroy() noexcept
{
    if(handle_)
    {
        vkDestroyImageView(logical_device_->handle_, handle_, context_->allocator_.get());
        handle_ = nullptr;
    }
}

std::error_code sgf::rhi::VulkanImageView::create(const ImageViewCreateInfo& create_info) noexcept
{
    const auto* vulkan_context = static_cast<const VulkanContext*>(create_info.context);
    const auto* vulkan_logical_device = static_cast<const VulkanLogicalDevice*>(create_info.logical_device);
    const auto* vulkan_image = static_cast<const VulkanImage*>(create_info.image);

    context_ = vulkan_context;
    logical_device_ = vulkan_logical_device;

    VkImageViewCreateInfo view_info {};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = vulkan_image->handle_;
    view_info.viewType = toVkImageViewType(create_info.type);
    view_info.format = toVkFormat(create_info.format);
    view_info.components = toVkComponentMapping(create_info.component_mapping);

    view_info.subresourceRange.aspectMask = toVkImageAspect(create_info.aspect_mask);
    view_info.subresourceRange.baseMipLevel = create_info.base_mip_level;
    view_info.subresourceRange.levelCount = create_info.levels_count;
    view_info.subresourceRange.baseArrayLayer = create_info.base_array_layer;
    view_info.subresourceRange.layerCount = create_info.layers_count;

    if(vkCreateImageView(logical_device_->handle_, &view_info, context_->allocator_.get(), &handle_) != VK_SUCCESS)
    {
        return VulkanError::kCannotCreateImageView;
    }

    return {};
}
