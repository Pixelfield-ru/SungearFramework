//
// Created by stuka on 03.09.2026.
//

#pragma once

#include <vector>

#include "shader_stage.h"

namespace sgf::rhi
{
    struct ICompiledShader
    {
        using allocator_type = std::pmr::polymorphic_allocator<>;

        core::Vector<ShaderStage> stages;

        ICompiledShader() = default;
        ICompiledShader(const allocator_type& alloc) :
            stages(alloc)
        { }
        ICompiledShader(const ICompiledShader&) = default;
        ICompiledShader(const ICompiledShader& other, const allocator_type& alloc) :
            stages(other.stages, alloc)
        { }
        ICompiledShader(ICompiledShader&&) = default;
        ICompiledShader(ICompiledShader&& other, const allocator_type& alloc) :
            stages(std::move(other.stages), alloc)
        { }

        virtual ~ICompiledShader() = default;

        virtual void getIR(ShaderStageType stage_type, const void** ir_buffer, size_t* ir_buffer_size) = 0;

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return stages.get_allocator();
        }

        ICompiledShader& operator=(const ICompiledShader&) = default;
        ICompiledShader& operator=(ICompiledShader&&) = default;
    };
}
