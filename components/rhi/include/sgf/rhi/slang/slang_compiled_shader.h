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

        void getIR(ShaderStageType stage_type, const void** ir_buffer, size_t* ir_buffer_size) override;
    };
}
