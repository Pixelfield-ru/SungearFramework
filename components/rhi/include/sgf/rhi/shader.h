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
        const IContext* context {};
        const ILogicalDevice* logical_device {};
        core::Vector<ShaderStageCreateInfo> stages_create_info;

        std::pmr::memory_resource* memory_resource = std::pmr::get_default_resource();
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
