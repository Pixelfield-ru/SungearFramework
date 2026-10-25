//
// Created by stuka on 12.10.2026.
//

#pragma once

#include <unordered_map>
#include <slang-com-ptr.h>

#include "sgf/core/memory.h"
#include "sgf/rhi/compiled_shader.h"

namespace sgf::rhi
{
    struct SlangCompiledShader final : ICompiledShader
    {
        struct Stage
        {
            Slang::ComPtr<slang::IEntryPoint> entry_point;
            Slang::ComPtr<slang::IBlob> spirv_code;
        };

        Slang::ComPtr<slang::ISession> session;
        Slang::ComPtr<slang::IModule> module;
        Slang::ComPtr<slang::IBlob> code_cache;

        core::UnorderedMap<ShaderStageType, Stage> entry_points;

        SlangCompiledShader() = default;
        explicit SlangCompiledShader(const allocator_type& allocator) :
            ICompiledShader(allocator),
            entry_points(allocator)
        { }
        SlangCompiledShader(const SlangCompiledShader&) = delete;
        SlangCompiledShader(SlangCompiledShader&&) noexcept = default;
        SlangCompiledShader(SlangCompiledShader&& other, const allocator_type& allocator) :
            ICompiledShader(std::move(other), allocator),
            session(std::move(other.session)),
            module(std::move(other.module)),
            code_cache(std::move(other.code_cache)),
            entry_points(std::move(other.entry_points), allocator)
        { }

        void getIR(ShaderStageType stage_type, const void** ir_buffer, size_t* ir_buffer_size) override;

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return entry_points.get_allocator();
        }

        SlangCompiledShader& operator=(const SlangCompiledShader&) = delete;
        SlangCompiledShader& operator=(SlangCompiledShader&&) = default;
    };
}
