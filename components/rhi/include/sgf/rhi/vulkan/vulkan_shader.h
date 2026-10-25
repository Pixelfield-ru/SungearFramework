//
// Created by stuka on 16.09.2026.
//

#pragma once

#include <vulkan/vulkan.h>

#include "sgf/rhi/shader.h"
#include "vulkan_errors.h"

namespace sgf::rhi
{
    struct VulkanContext;

    struct VulkanShader final : IShader
    {
        friend struct VulkanLogicalDevice;
        friend struct VulkanPipelineState;

        using allocator_type = std::pmr::polymorphic_allocator<>;

        VulkanShader() noexcept = default;
        explicit VulkanShader(const allocator_type& allocator) noexcept;
        VulkanShader(const VulkanShader&) = delete;
        VulkanShader(VulkanShader&& other) noexcept;
        VulkanShader(VulkanShader&& other, const allocator_type& allocator);
        ~VulkanShader() noexcept override;

        [[nodiscard]] bool valid() const noexcept override;

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return stages_.get_allocator();
        }

        VulkanShader& operator=(const VulkanShader&) = delete;
        VulkanShader& operator=(VulkanShader&&) noexcept;

    private:
        struct Stage final
        {
            VkShaderModule module = VK_NULL_HANDLE;
            ShaderStage stage_info;
            VkPipelineShaderStageCreateInfo pipeline_stage_info {};
        };

        const VulkanContext* context_ {};
        const VulkanLogicalDevice* logical_device_ {};

        core::Vector<Stage> stages_;

        std::pmr::memory_resource* memory_resource_ {};

        void destroy() noexcept override;
        [[nodiscard]] std::error_code create(const ShaderCreateInfo& create_info) noexcept override;
    };
}
