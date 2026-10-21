//
// Created by stuka on 03.09.2026.
//

#pragma once

#include <unordered_map>
#include <vector>
#include <filesystem>
#include <memory>

#include "shader_stage.h"
#include "sgf/core/memory.h"

namespace sgf::rhi
{
    struct ICompiledShader;
    struct IContext;

    struct IShaderCompiler
    {
        core::UnorderedMap<ShaderStageType, core::String> standard_entry_points {
            { ShaderStageType::kVertex, "vertexMain" },
            { ShaderStageType::kFragment, "fragmentMain" },
            { ShaderStageType::kCompute, "computeMain" },
            { ShaderStageType::kTessellationControl, "tesselationControlMain" },
            { ShaderStageType::kTessellationEval, "tesselationEvalMain" },
            { ShaderStageType::kGeometry, "geometryMain" }
        };

        core::Vector<std::filesystem::path> search_paths;
        std::filesystem::path cache_path = "cache/shaders";

        std::pmr::memory_resource* memory_resource = std::pmr::get_default_resource();

        virtual ~IShaderCompiler() = default;

        virtual std::error_code compileShader(const IContext& context, ICompiledShader& compiled_shader,
                                              const std::filesystem::path& shader_path) = 0;
    };
}
