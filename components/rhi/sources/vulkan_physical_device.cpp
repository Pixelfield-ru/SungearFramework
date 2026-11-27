//
// Created by stuka on 16.09.2026.
//

#include "sgf/rhi/vulkan/vulkan_physical_device.h"

#include <expected>

#include "sgf/core/memory.h"
#include "sgf/rhi/vulkan/vulkan_errors.h"
#include "sgf/rhi/vulkan/vulkan_surface.h"
#include "sgf/rhi/vulkan/vulkan_types.h"

sgf::rhi::VulkanPhysicalDevice::VulkanPhysicalDevice(const allocator_type& allocator) :
    IPhysicalDevice(allocator)
{
}

sgf::rhi::VulkanPhysicalDevice::VulkanPhysicalDevice(VulkanPhysicalDevice&& other) noexcept :
    IPhysicalDevice(std::move(other)),
    handle_(std::exchange(other.handle_, VK_NULL_HANDLE)),
    features_(std::exchange(other.features_, {})),
    queue_families_indices_(std::exchange(other.queue_families_indices_, {}))
{

}

sgf::rhi::VulkanPhysicalDevice::VulkanPhysicalDevice(VulkanPhysicalDevice&& other, const allocator_type& allocator) :
    IPhysicalDevice(std::move(other), allocator),
    handle_(std::exchange(other.handle_, VK_NULL_HANDLE)),
    features_(std::exchange(other.features_, {})),
    queue_families_indices_(std::exchange(other.queue_families_indices_, {}))
{
}

bool sgf::rhi::VulkanPhysicalDevice::valid() const noexcept
{
    return handle_ != nullptr;
}

std::expected<sgf::rhi::SurfaceCapabilities, std::error_code> sgf::rhi::VulkanPhysicalDevice::querySurfaceCapabilities(
    const ISurface& surface, const allocator_type& alloc) const
{
    if(!valid())
    {
        return std::unexpected(VulkanError::kInvalidDevice);
    }

    const auto& vulkan_surface = static_cast<const VulkanSurface&>(surface);
    if(!vulkan_surface.handle_)
    {
        return std::unexpected(VulkanError::kInvalidArgument);
    }

    return querySurfaceCapabilities(vulkan_surface.handle_, alloc);
}

std::expected<sgf::rhi::SurfaceCapabilities, std::error_code> sgf::rhi::VulkanPhysicalDevice::querySurfaceCapabilities(
    VkSurfaceKHR surface, const allocator_type& alloc) const
{
    if(!valid())
    {
        return std::unexpected(VulkanError::kInvalidDevice);
    }

    if(!surface)
    {
        return std::unexpected(VulkanError::kInvalidArgument);
    }

    VkSurfaceCapabilitiesKHR surface_caps {};
    if(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(handle_, surface, &surface_caps) != VK_SUCCESS)
    {
        return std::unexpected(VulkanError::kCannotQuerySurface);
    }

    std::uint32_t format_count = 0;
    if(vkGetPhysicalDeviceSurfaceFormatsKHR(handle_, surface, &format_count, nullptr) != VK_SUCCESS)
    {
        return std::unexpected(VulkanError::kCannotQuerySurface);
    }

    std::uint32_t present_mode_count = 0;
    if(vkGetPhysicalDeviceSurfacePresentModesKHR(handle_, surface, &present_mode_count, nullptr) != VK_SUCCESS)
    {
        return std::unexpected(VulkanError::kCannotQuerySurface);
    }

    std::byte stack[core::stack_optimal_size];
    std::pmr::monotonic_buffer_resource stack_resource(stack, sizeof(stack));

    core::Vector<VkSurfaceFormatKHR> formats(format_count, &stack_resource);
    if(format_count != 0 &&
       vkGetPhysicalDeviceSurfaceFormatsKHR(handle_, surface, &format_count, formats.data()) != VK_SUCCESS)
    {
        return std::unexpected(VulkanError::kCannotQuerySurface);
    }

    core::Vector<VkPresentModeKHR> present_modes(present_mode_count, &stack_resource);
    if(present_mode_count != 0 &&
       vkGetPhysicalDeviceSurfacePresentModesKHR(handle_, surface, &present_mode_count, present_modes.data()) != VK_SUCCESS)
    {
        return std::unexpected(VulkanError::kCannotQuerySurface);
    }

    SurfaceCapabilities capabilities(alloc);
    capabilities.present_queue_families.reserve(std::to_underlying(QueueFamilyType::kCount));

    for(std::uint32_t type = 0; type < std::to_underlying(QueueFamilyType::kCount); ++type)
    {
        if(static_cast<QueueFamilyType>(type) == QueueFamilyType::kPresent) continue;

        const auto& family_index = queue_families_indices_[type];
        if(!family_index) continue;

        VkBool32 family_supports_present = VK_FALSE;
        if(vkGetPhysicalDeviceSurfaceSupportKHR(handle_, *family_index, surface, &family_supports_present) != VK_SUCCESS)
        {
            return std::unexpected(VulkanError::kCannotQuerySurface);
        }

        if(family_supports_present)
        {
            capabilities.present_queue_families.push_back(static_cast<QueueFamilyType>(type));
        }
    }

    capabilities.current_extent = { surface_caps.currentExtent.width, surface_caps.currentExtent.height };
    capabilities.min_extent = { surface_caps.minImageExtent.width, surface_caps.minImageExtent.height };
    capabilities.max_extent = { surface_caps.maxImageExtent.width, surface_caps.maxImageExtent.height };
    capabilities.current_transform = fromVkSurfaceTransform(surface_caps.currentTransform);
    capabilities.min_image_count = surface_caps.minImageCount;
    capabilities.max_image_count = surface_caps.maxImageCount;

    capabilities.formats.reserve(formats.size());
    for(const auto& format : formats)
    {
        capabilities.formats.push_back(SurfaceFormat {
            fromVkFormat(format.format),
            fromVkColorSpace(format.colorSpace)
        });
    }

    capabilities.present_modes.reserve(present_modes.size());
    for(const auto present_mode : present_modes)
    {
        const PresentMode mode = fromVkPresentMode(present_mode);
        if(mode == PresentMode::kUnknown) continue;
        capabilities.present_modes.push_back(mode);
    }

    return capabilities;
}

sgf::rhi::VulkanPhysicalDevice& sgf::rhi::VulkanPhysicalDevice::operator=(VulkanPhysicalDevice&& other) noexcept
{
    if(this == std::addressof(other)) return *this;

    IPhysicalDevice::operator=(std::move(other));
    handle_ = std::exchange(other.handle_, VK_NULL_HANDLE);
    features_ = std::exchange(other.features_, {});
    queue_families_indices_ = std::exchange(other.queue_families_indices_, {});

    return *this;
}

std::int32_t sgf::rhi::VulkanPhysicalDevice::findMemoryIndex(VkMemoryRequirements requirements,
                                                        VkMemoryPropertyFlags properties) const noexcept
{
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(handle_, &mem_props);

    std::uint32_t type_filter = requirements.memoryTypeBits;

    // get the best memory heap
    for(std::uint32_t i = 0; i < mem_props.memoryTypeCount; i++)
    {
        if((type_filter & (1 << i)) && (mem_props.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    return -1;
}