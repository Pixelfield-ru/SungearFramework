//
// Created by stuka on 02.09.2026.
//

#pragma once

#include <filesystem>
#include <slang-com-ptr.h>

#include "sgf/rhi/shader_compiler.h"

namespace sgf::rhi
{
    struct SlangShaderCompiler final : public IShaderCompiler
    {
        std::error_code compileShader(const IContext& context, ICompiledShader& compiled_shader,
                                      const std::filesystem::path& shader_path) override;

        static Slang::ComPtr<slang::IGlobalSession> globalSession() noexcept;
    };
}