//
// Created by stuka on 16.09.2026.
//

#include "sgf/rhi/vulkan/vulkan_physical_device.h"

sgf::rhi::VulkanPhysicalDevice::VulkanPhysicalDevice(VulkanPhysicalDevice&& other) noexcept :
    handle_(std::exchange(other.handle_, VK_NULL_HANDLE)),
    features_(std::exchange(other.features_, {})),
    surface_support_(std::move(other.surface_support_)),
    queue_families_indices_(std::exchange(other.queue_families_indices_, {}))
{

}

bool sgf::rhi::VulkanPhysicalDevice::valid() const noexcept
{
    return handle_ != nullptr;
}

sgf::rhi::VulkanPhysicalDevice& sgf::rhi::VulkanPhysicalDevice::operator=(VulkanPhysicalDevice&& other) noexcept
{
    if(this == std::addressof(other)) return *this;

    handle_ = std::exchange(other.handle_, VK_NULL_HANDLE);
    features_ = std::exchange(other.features_, {});
    surface_support_ = std::move(other.surface_support_);
    queue_families_indices_ = std::exchange(other.queue_families_indices_, {});

    return *this;
}

std::int32_t sgf::rhi::VulkanPhysicalDevice::findMemoryIndex(VkMemoryRequirements requirements,
                                                        VkMemoryPropertyFlags properties) const noexcept
{
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(handle_, &mem_props);

    std::uint32_t type_filter = requirements.memoryTypeBits;

    // get best memory heap
    for(std::uint32_t i = 0; i < mem_props.memoryTypeCount; i++)
    {
        if((type_filter & (1 << i)) && (mem_props.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    return -1;
}
