//
// Created by stuka on 16.09.2026.
//

#pragma once

#include <vector>
#include <system_error>

#include "shader_stage.h"

namespace sgf::rhi
{
    struct IContext;
    struct ILogicalDevice;

    struct ShaderStageCreateInfo
    {
        ShaderStage stage;
        const void* code_buffer {};
        size_t code_buffer_size {};
    };

    struct ShaderCreateInfo
    {
        using allocator_type = std::pmr::polymorphic_allocator<>;

        const IContext* context {};
        const ILogicalDevice* logical_device {};
        core::Vector<ShaderStageCreateInfo> stages_create_info;

        ShaderCreateInfo() = default;
        explicit ShaderCreateInfo(const allocator_type& allocator) :
            stages_create_info(allocator)
        { }
        ShaderCreateInfo(const ShaderCreateInfo&) = default;
        ShaderCreateInfo(const ShaderCreateInfo& other, const allocator_type& allocator) :
            context(other.context),
            logical_device(other.logical_device),
            stages_create_info(other.stages_create_info, allocator)
        { }
        ShaderCreateInfo(ShaderCreateInfo&&) noexcept = default;
        ShaderCreateInfo(ShaderCreateInfo&& other, const allocator_type& allocator) :
            context(other.context),
            logical_device(other.logical_device),
            stages_create_info(std::move(other.stages_create_info), allocator)
        { }

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return stages_create_info.get_allocator();
        }

        ShaderCreateInfo& operator=(const ShaderCreateInfo&) = default;
        ShaderCreateInfo& operator=(ShaderCreateInfo&&) noexcept = default;
    };

    struct IShader
    {
        virtual ~IShader() = default;

        [[nodiscard]] virtual bool valid() const noexcept = 0;

    private:
        virtual void destroy() = 0;
        [[nodiscard]] virtual std::error_code create(const ShaderCreateInfo& create_info) = 0;
    };
}
