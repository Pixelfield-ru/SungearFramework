//
// Created by stuka on 17.11.2026.
//

#include "sgf/rhi/vulkan/vulkan_surface.h"

#include <GLFW/glfw3.h>

#include "sgf/platform/window.h"
#include "sgf/rhi/vulkan/vulkan_context.h"
#include "sgf/rhi/vulkan/vulkan_errors.h"

sgf::rhi::VulkanSurface::VulkanSurface(VulkanSurface&& other) noexcept :
    context_(std::exchange(other.context_, nullptr)),
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

    context_ = std::exchange(other.context_, nullptr);
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
    if(!create_info.context || !create_info.window)
    {
        return VulkanError::kInvalidArgument;
    }

    const auto* vulkan_context = static_cast<const VulkanContext*>(create_info.context);
    if(!vulkan_context->valid())
    {
        return VulkanError::kInvalidContext;
    }

    auto* window = static_cast<GLFWwindow*>(create_info.window->handle());
    if(!window)
    {
        return VulkanError::kInvalidArgument;
    }

    context_ = vulkan_context;

    if(glfwCreateWindowSurface(vulkan_context->instance_, window, vulkan_context->allocator_.get(), &handle_) != VK_SUCCESS)
    {
        context_ = {};
        return VulkanError::kCannotCreateSurface;
    }

    return {};
}
