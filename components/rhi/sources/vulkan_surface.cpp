//
// Created by stuka on 17.11.2026.
//

#include "sgf/rhi/vulkan/vulkan_surface.h"

#include "sgf/rhi/vulkan/vulkan_context.h"

sgf::rhi::VulkanSurface::VulkanSurface(VulkanSurface&& other) noexcept :
    handle_(std::exchange(other.handle_, {}))
{

}

sgf::rhi::VulkanSurface::~VulkanSurface() noexcept
{
    destroy();
}

sgf::rhi::VulkanSurface& sgf::rhi::VulkanSurface::operator=(VulkanSurface&& other) noexcept
{
    if(std::addressof(other) == this) return *this;

    destroy();

    handle_ = std::exchange(other.handle_, {});

    return *this;
}

void sgf::rhi::VulkanSurface::destroy()
{
    if(handle_)
    {
        vkDestroySurfaceKHR(context_->instance_, handle_, context_->allocator_.get());
        handle_ = {};
    }
}

std::error_code sgf::rhi::VulkanSurface::create(const SurfaceCreateInfo& create_info)
{
    const auto* vulkan_context = static_cast<const VulkanContext*>(create_info.context);

    context_ = vulkan_context;

    // todo: create surface

    return {};
}
