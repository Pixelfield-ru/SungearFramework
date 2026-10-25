//
// Created by stuka on 11.09.2026.
//

#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include "sgf/rhi/swapchain.h"
#include "vulkan_errors.h"
#include "sgf/core/memory.h"

namespace sgf::rhi
{
    struct VulkanLogicalDevice;
    struct VulkanContext;
    struct VulkanImageView;

    struct VulkanSwapchain final : ISwapchain
    {
        friend struct VulkanLogicalDevice;

        using allocator_type = std::pmr::polymorphic_allocator<>;

        VulkanSwapchain() noexcept = default;
        explicit VulkanSwapchain(const allocator_type& allocator) noexcept;
        VulkanSwapchain(const VulkanSwapchain&) = delete;
        VulkanSwapchain(VulkanSwapchain&& other) noexcept;
        VulkanSwapchain(VulkanSwapchain&& other, const allocator_type& allocator);
        ~VulkanSwapchain() noexcept override;

        [[nodiscard]] bool valid() const noexcept override;
        [[nodiscard]] glm::ivec2 extent() const noexcept override;
        [[nodiscard]] Format format() const noexcept override;
        [[nodiscard]] std::uint16_t imagesCount() const noexcept override;
        [[nodiscard]] IImageView* imageView(std::uint16_t idx) noexcept override;
        [[nodiscard]] std::expected<AcquiredFrame, std::error_code> acquireNextFrame() noexcept override;
        [[nodiscard]] std::error_code present(const PresentInfo& present_info) noexcept override;

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return images_view_.get_allocator();
        }

        VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;
        VulkanSwapchain& operator=(VulkanSwapchain&& other) noexcept;

    private:
        const VulkanContext* context_ {};
        const VulkanLogicalDevice* logical_device_ {};

        VkSwapchainKHR handle_ {};

        VkSurfaceFormatKHR surface_format_ {};
        VkPresentModeKHR present_mode_ = VK_PRESENT_MODE_MAILBOX_KHR;
        VkExtent2D extent_ {};

        core::Vector<VulkanImageView> images_view_;

        core::Vector<VkSemaphore> image_available_semaphores_;
        core::Vector<VkSemaphore> render_finished_semaphores_;
        core::Vector<VkFence> busy_frames_fences_;

        std::uint32_t current_frame_idx_ {};

        void destroy() noexcept override;
        [[nodiscard]] std::error_code create(const SwapchainCreateInfo& create_info) noexcept override;
    };
}
