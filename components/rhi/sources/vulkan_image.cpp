//
// Created by stuka on 14.09.2026.
//

#include "sgf/rhi/vulkan/vulkan_image.h"

#include "sgf/rhi/vulkan/vulkan_context.h"
#include "sgf/rhi/vulkan/vulkan_errors.h"
#include "sgf/rhi/vulkan/vulkan_logical_device.h"
#include "sgf/rhi/vulkan/vulkan_physical_device.h"
#include "sgf/rhi/vulkan/vulkan_types.h"

sgf::rhi::VulkanImage::VulkanImage(VkImage handle) noexcept
{
    handle_ = handle;
    owns_image_ = false;
}

sgf::rhi::VulkanImage::VulkanImage(VulkanImage&& other) noexcept :
    context_(std::exchange(other.context_, {})),
    logical_device_(std::exchange(other.logical_device_, {})),
    handle_(std::exchange(other.handle_, VK_NULL_HANDLE)),
    memory_(std::exchange(other.memory_, VK_NULL_HANDLE)),
    owns_image_(std::exchange(other.owns_image_, false)),
    view_(std::exchange(other.view_, {}))
{

}

sgf::rhi::VulkanImage::~VulkanImage() noexcept
{
    destroy();
}

std::error_code sgf::rhi::VulkanImage::update(const ImageUpdateInfo& update_info) noexcept
{
    return logical_device_->updateImage(*this, update_info);
}

sgf::rhi::IImageView const *const sgf::rhi::VulkanImage::view() const noexcept
{
    return &view_;
}

sgf::rhi::IImageView *const sgf::rhi::VulkanImage::view() noexcept
{
    return &view_;
}

bool sgf::rhi::VulkanImage::valid() const noexcept
{
    return handle_ != nullptr;
}

sgf::rhi::VulkanImage& sgf::rhi::VulkanImage::operator=(VulkanImage&& other) noexcept
{
    if(this == std::addressof(other)) return *this;

    destroy();

    context_ = std::exchange(other.context_, {});
    logical_device_ = std::exchange(other.logical_device_, {});
    handle_ = std::exchange(other.handle_, VK_NULL_HANDLE);
    memory_ = std::exchange(other.memory_, VK_NULL_HANDLE);
    owns_image_ = std::exchange(other.owns_image_, false);
    view_ = std::exchange(other.view_, {});

    return *this;
}

void sgf::rhi::VulkanImage::destroy() noexcept
{
    view_.destroy();

    if(!owns_image_) return;

    if(handle_)
    {
        vkDestroyImage(logical_device_->handle_, handle_, context_->allocator_.get());
        handle_ = VK_NULL_HANDLE;
    }

    if(memory_)
    {
        vkFreeMemory(logical_device_->handle_, memory_, context_->allocator_.get());
        memory_ = VK_NULL_HANDLE;
    }
}

std::error_code sgf::rhi::VulkanImage::create(const ImageCreateInfo& create_info) noexcept
{
    const auto* vulkan_context = static_cast<const VulkanContext*>(create_info.context);
    auto* vulkan_logical_device = static_cast<VulkanLogicalDevice*>(create_info.logical_device);

    context_ = vulkan_context;
    logical_device_ = vulkan_logical_device;

    owns_image_ = true;
    format_ = create_info.format;

    VkImageCreateInfo vulkan_image_info {};
    vulkan_image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    vulkan_image_info.imageType = toVkImageType(create_info.type);
    vulkan_image_info.format = toVkFormat(create_info.format);
    vulkan_image_info.extent = { create_info.extent.x, create_info.extent.y, create_info.extent.z };
    vulkan_image_info.mipLevels = create_info.mip_levels;
    vulkan_image_info.arrayLayers = create_info.array_layers;
    vulkan_image_info.samples = toVkSampleCount(create_info.sample_count);
    vulkan_image_info.tiling = toVkImageTiling(create_info.tiling);
    vulkan_image_info.usage = toVkImageUsage(create_info.usage);
    vulkan_image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // todo: layout choose
    vulkan_image_info.initialLayout = toVkImageLayout(layout_);

    if(vkCreateImage(vulkan_logical_device->handle_, &vulkan_image_info, vulkan_context->allocator_.get(), &handle_) != VK_SUCCESS)
    {
        return VulkanError::kCannotCreateImage;
    }

    VkMemoryRequirements mem_reqs;
    vkGetImageMemoryRequirements(vulkan_logical_device->handle_, handle_, &mem_reqs);

    const auto memory_index = vulkan_logical_device->physical_device_->findMemoryIndex(mem_reqs, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkMemoryAllocateInfo alloc{};
    alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc.allocationSize = mem_reqs.size;
    alloc.memoryTypeIndex = memory_index;

    if(vkAllocateMemory(vulkan_logical_device->handle_, &alloc, vulkan_context->allocator_.get(), &memory_) != VK_SUCCESS)
    {
        return VulkanError::kCannotAllocateMemory;
    }

    if(vkBindImageMemory(vulkan_logical_device->handle_, handle_, memory_, 0) != VK_SUCCESS)
    {
        return VulkanError::kCannotBindMemory;
    }

    ImageViewCreateInfo view_create_info;
    view_create_info.context = vulkan_context;
    view_create_info.logical_device = vulkan_logical_device;
    view_create_info.image = this;
    view_create_info.type = create_info.type;
    view_create_info.format = create_info.format;
    view_create_info.aspect_mask = ImageAspect::kColor;
    view_create_info.base_mip_level = 0;
    view_create_info.levels_count = create_info.mip_levels;
    view_create_info.base_array_layer = 0;
    view_create_info.layers_count = create_info.array_layers;

    if(const auto res = view_.create(view_create_info))
    {
        return res;
    }

    return {};
}
