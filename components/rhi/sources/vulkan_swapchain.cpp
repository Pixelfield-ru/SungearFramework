//
// Created by stuka on 11.09.2026.
//

#include "sgf/rhi/vulkan/vulkan_swapchain.h"

#include <algorithm>
#include <limits>

#include "sgf/core/errors.h"
#include "sgf/platform/window.h"
#include "sgf/rhi/vulkan/vulkan_context.h"
#include "sgf/rhi/vulkan/vulkan_surface.h"
#include "sgf/rhi/vulkan/vulkan_image.h"
#include "sgf/rhi/vulkan/vulkan_image_view.h"
#include "sgf/rhi/vulkan/vulkan_logical_device.h"
#include "sgf/rhi/vulkan/vulkan_physical_device.h"

#include "sgf/rhi/vulkan/vulkan_types.h"
#include "sgf/rhi/errors.h"

sgf::rhi::VulkanSwapchain::VulkanSwapchain(const allocator_type& allocator) noexcept :
    images_view_(allocator),
    image_available_semaphores_(allocator),
    render_finished_semaphores_(allocator),
    busy_frames_fences_(allocator)
{
}

sgf::rhi::VulkanSwapchain::VulkanSwapchain(VulkanSwapchain&& other) noexcept :
    context_(std::exchange(other.context_, nullptr)),
    logical_device_(std::exchange(other.logical_device_, nullptr)),
    handle_(std::exchange(other.handle_, VK_NULL_HANDLE)),
    surface_format_(std::exchange(other.surface_format_, {})),
    present_mode_(std::exchange(other.present_mode_, {})),
    extent_(std::exchange(other.extent_, {})),
    images_view_(std::move(other.images_view_)),
    image_available_semaphores_(std::move(other.image_available_semaphores_)),
    render_finished_semaphores_(std::move(other.render_finished_semaphores_)),
    busy_frames_fences_(std::move(other.busy_frames_fences_))
{

}

sgf::rhi::VulkanSwapchain::VulkanSwapchain(VulkanSwapchain&& other, const allocator_type& allocator) :
    context_(std::exchange(other.context_, nullptr)),
    logical_device_(std::exchange(other.logical_device_, nullptr)),
    handle_(std::exchange(other.handle_, VK_NULL_HANDLE)),
    surface_format_(std::exchange(other.surface_format_, {})),
    present_mode_(std::exchange(other.present_mode_, {})),
    extent_(std::exchange(other.extent_, {})),
    images_view_(std::move(other.images_view_), allocator),
    image_available_semaphores_(std::move(other.image_available_semaphores_), allocator),
    render_finished_semaphores_(std::move(other.render_finished_semaphores_), allocator),
    busy_frames_fences_(std::move(other.busy_frames_fences_), allocator)
{
}

sgf::rhi::VulkanSwapchain::~VulkanSwapchain() noexcept
{
    destroy();
}

bool sgf::rhi::VulkanSwapchain::valid() const noexcept
{
    return handle_ != nullptr;
}

glm::ivec2 sgf::rhi::VulkanSwapchain::extent() const noexcept
{
    return { extent_.width, extent_.height };
}

sgf::rhi::Format sgf::rhi::VulkanSwapchain::format() const noexcept
{
    return fromVkFormat(surface_format_.format);
}

std::uint16_t sgf::rhi::VulkanSwapchain::imagesCount() const noexcept
{
    return images_view_.size();
}

sgf::rhi::IImageView* sgf::rhi::VulkanSwapchain::imageView(std::uint16_t idx) noexcept
{
    return &images_view_[idx];
}

std::expected<sgf::rhi::AcquiredFrame, std::error_code> sgf::rhi::VulkanSwapchain::acquireNextFrame() noexcept
{
    // Do not reset the fence here: if acquire succeeds but submit fails later,
    // an early reset leaves the fence unsignaled forever.
    vkWaitForFences(logical_device_->handle_, 1,
                            &busy_frames_fences_[current_frame_idx_], VK_TRUE, UINT64_MAX);

    uint32_t image_index;
    const VkResult acquire_result = vkAcquireNextImageKHR(
        logical_device_->handle_, handle_, UINT64_MAX,
        image_available_semaphores_[current_frame_idx_], VK_NULL_HANDLE, &image_index);

    if(acquire_result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        return std::unexpected(Error::kNotOptimalSwapchain);
    }

    if(acquire_result != VK_SUCCESS && acquire_result != VK_SUBOPTIMAL_KHR)
    {
        return std::unexpected(VulkanError::kCannotAcquireNextImage);
    }

    AcquiredFrame frame;
    frame.image_index = image_index;
    frame.image_view = &images_view_[image_index];

    return frame;
}

std::error_code sgf::rhi::VulkanSwapchain::present(const PresentInfo& present_info) noexcept
{
    const VkSwapchainKHR swapchains[] = { handle_ };

    VkPresentInfoKHR vulkan_present_info {};
    vulkan_present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    vulkan_present_info.waitSemaphoreCount = 1;
    vulkan_present_info.pWaitSemaphores = &render_finished_semaphores_[present_info.frame->image_index];
    vulkan_present_info.swapchainCount = 1;
    vulkan_present_info.pSwapchains = swapchains;
    vulkan_present_info.pImageIndices = &present_info.frame->image_index;
    vulkan_present_info.pResults = nullptr; // Optional

    const auto present_result = vkQueuePresentKHR(
        logical_device_->queues_[std::to_underlying(present_info.queue_family)], &vulkan_present_info);

    // Always advance frame slot — otherwise fence/semaphore indexing desyncs across recreate.
    current_frame_idx_ = (current_frame_idx_ + 1) % image_available_semaphores_.size();

    if(present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR)
    {
        return Error::kNotOptimalSwapchain;
    }

    if(present_result != VK_SUCCESS)
    {
        return VulkanError::kCannotPresentFrame;
    }

    return {};
}

sgf::rhi::VulkanSwapchain& sgf::rhi::VulkanSwapchain::operator=(VulkanSwapchain&& other) noexcept
{
    if(this == std::addressof(other)) return *this;

    destroy();

    context_ = std::exchange(other.context_, nullptr);
    logical_device_ = std::exchange(other.logical_device_, nullptr);
    handle_ = std::exchange(other.handle_, VK_NULL_HANDLE);
    surface_format_ = std::exchange(other.surface_format_, {});
    present_mode_ = std::exchange(other.present_mode_, {});
    extent_ = std::exchange(other.extent_, {});
    images_view_ = std::move(other.images_view_);
    image_available_semaphores_ = std::move(other.image_available_semaphores_);
    render_finished_semaphores_ = std::move(other.render_finished_semaphores_);
    busy_frames_fences_ = std::move(other.busy_frames_fences_);

    return *this;
}

void sgf::rhi::VulkanSwapchain::destroy() noexcept
{
    for(auto& semaphore : render_finished_semaphores_)
    {
        vkDestroySemaphore(logical_device_->handle_, semaphore, context_->allocator_.get());
    }
    render_finished_semaphores_.clear();

    for(const auto& semaphore : image_available_semaphores_)
    {
        vkDestroySemaphore(logical_device_->handle_, semaphore, context_->allocator_.get());
    }
    image_available_semaphores_.clear();

    for(const auto& fence : busy_frames_fences_)
    {
        vkDestroyFence(logical_device_->handle_, fence, context_->allocator_.get());
    }
    busy_frames_fences_.clear();

    // destroying images views before swapchain
    images_view_.clear();

    if(handle_)
    {
        vkDestroySwapchainKHR(logical_device_->handle_, handle_, context_->allocator_.get());
        handle_ = VK_NULL_HANDLE;
    }

    current_frame_idx_ = 0;
}

std::error_code sgf::rhi::VulkanSwapchain::create(const SwapchainCreateInfo& create_info) noexcept
{
    if(!create_info.context || !create_info.logical_device || !create_info.physical_device ||
       !create_info.surface || !create_info.window)
    {
        return VulkanError::kInvalidArgument;
    }

    auto* vulkan_context = static_cast<const VulkanContext*>(create_info.context);
    auto* vulkan_logical_device = static_cast<const VulkanLogicalDevice*>(create_info.logical_device);
    const auto* vulkan_physical_device = static_cast<const VulkanPhysicalDevice*>(create_info.physical_device);
    const auto& vulkan_surface = static_cast<const VulkanSurface&>(*create_info.surface);

    logical_device_ = vulkan_logical_device;
    context_ = vulkan_context;

    const auto capabilities_result = vulkan_physical_device->querySurfaceCapabilities(*create_info.surface, get_allocator());
    if(!capabilities_result)
    {
        return capabilities_result.error();
    }

    const SurfaceCapabilities& capabilities = *capabilities_result;

    if(capabilities.formats.empty() || capabilities.present_modes.empty() || capabilities.present_queue_families.empty())
    {
        return VulkanError::kIncompatibleDevice;
    }

    // ============================= selecting format & color space

    const SurfaceFormat* exact = nullptr;
    const SurfaceFormat* format_only = nullptr;

    for(const auto& available : capabilities.formats)
    {
        const bool format_ok = available.format == create_info.preferred_format;
        const bool space_ok = available.color_space == create_info.preferred_color_space;

        if(format_ok && space_ok)
        {
            exact = &available;
            break;
        }

        if(format_ok && !format_only)
        {
            format_only = &available;
        }
    }

    const SurfaceFormat& chosen_format = exact ? *exact : format_only ? *format_only : capabilities.formats.front();
    surface_format_.format = toVkFormat(chosen_format.format);
    surface_format_.colorSpace = toVkColorSpace(chosen_format.color_space);

    // ============================ choosing present mode
    const PresentMode preferred_present_mode = fromVkPresentMode(present_mode_);
    PresentMode chosen_present_mode = capabilities.present_modes.front();
    for(const auto available_present_mode : capabilities.present_modes)
    {
        if(available_present_mode == preferred_present_mode)
        {
            chosen_present_mode = preferred_present_mode;
            break;
        }
    }

    present_mode_ = toVkPresentMode(chosen_present_mode);

    // ============================= choosing extent

    if(capabilities.current_extent.x != std::numeric_limits<std::uint32_t>::max())
    {
        extent_ = { capabilities.current_extent.x, capabilities.current_extent.y };
    }
    else
    {
        std::int32_t width = 0;
        std::int32_t height = 0;
        create_info.window->framebufferSize(width, height);

        extent_ = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        extent_.width = std::max(capabilities.min_extent.x, std::min(capabilities.max_extent.x, extent_.width));
        extent_.height = std::max(capabilities.min_extent.y, std::min(capabilities.max_extent.y, extent_.height));
    }

    // ============================== choosing images count
    std::uint32_t image_count = capabilities.min_image_count + 1;

    if(capabilities.max_image_count > 0 && image_count > capabilities.max_image_count)
    {
        image_count = capabilities.max_image_count;
    }

    // ============================== creating swapchain
    VkSwapchainCreateInfoKHR swapchain_create_info{};
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.surface = vulkan_surface.handle_;
    swapchain_create_info.minImageCount = image_count;
    swapchain_create_info.imageFormat = surface_format_.format;
    swapchain_create_info.imageColorSpace = surface_format_.colorSpace;
    swapchain_create_info.imageExtent = extent_;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    const auto& graphics_index = vulkan_physical_device->queue_families_indices_[std::to_underlying(QueueFamilyType::kGraphics)];
    // todo: add queue to present selection
    const auto graphics_present = std::ranges::find(
        capabilities.present_queue_families, QueueFamilyType::kGraphics);
    if(!graphics_index || graphics_present == capabilities.present_queue_families.end())
    {
        return VulkanError::kIncompatibleDevice;
    }

    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.queueFamilyIndexCount = 0;
    swapchain_create_info.pQueueFamilyIndices = nullptr;

    swapchain_create_info.preTransform = toVkSurfaceTransform(capabilities.current_transform);
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode = present_mode_;
    swapchain_create_info.clipped = VK_TRUE;
    swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;

    if(vkCreateSwapchainKHR(logical_device_->handle_, &swapchain_create_info, context_->allocator_.get(), &handle_) != VK_SUCCESS)
    {
        return VulkanError::kCannotCreateSwapchain;
    }

    core::Vector<VkImage> swapchain_images;

    vkGetSwapchainImagesKHR(logical_device_->handle_, handle_, &image_count, nullptr);
    swapchain_images.resize(image_count);
    vkGetSwapchainImagesKHR(logical_device_->handle_, handle_, &image_count, swapchain_images.data());

    images_view_.resize(image_count);

    for(size_t i = 0; i < swapchain_images.size(); ++i)
    {
        VulkanImage image(swapchain_images[i]);

        ImageViewCreateInfo image_view_create_info;
        image_view_create_info.context = vulkan_context;
        image_view_create_info.logical_device = vulkan_logical_device;
        image_view_create_info.image = &image;
        image_view_create_info.format = fromVkFormat(surface_format_.format);
        image_view_create_info.type = ImageType::k2D;

        image_view_create_info.aspect_mask = ImageAspect::kColor;
        image_view_create_info.base_mip_level = 0;
        image_view_create_info.levels_count = 1;
        image_view_create_info.base_array_layer = 0;
        image_view_create_info.layers_count = 1;

        if(const auto res = images_view_[i].create(image_view_create_info))
        {
            return res;
        }
    }

    // creating render semaphores & fences
    // image_available / fences — per frame-in-flight
    // render_finished — per swapchain image (present may hold it until that image is re-acquired)
    image_available_semaphores_.resize(create_info.max_frames_in_flight);
    busy_frames_fences_.resize(create_info.max_frames_in_flight);
    render_finished_semaphores_.resize(swapchain_images.size());

    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for(size_t i = 0; i < create_info.max_frames_in_flight; ++i)
    {
        if(vkCreateSemaphore(logical_device_->handle_, &semaphore_info, vulkan_context->allocator_.get(), &image_available_semaphores_[i]) != VK_SUCCESS ||
           vkCreateFence(logical_device_->handle_, &fence_info, vulkan_context->allocator_.get(), &busy_frames_fences_[i]) != VK_SUCCESS)
        {
            return VulkanError::kCannotCreateSemaphore;
        }
    }

    for(size_t i = 0; i < render_finished_semaphores_.size(); ++i)
    {
        if(vkCreateSemaphore(logical_device_->handle_, &semaphore_info, vulkan_context->allocator_.get(), &render_finished_semaphores_[i]) != VK_SUCCESS)
        {
            return VulkanError::kCannotCreateSemaphore;
        }
    }

    return {};
}