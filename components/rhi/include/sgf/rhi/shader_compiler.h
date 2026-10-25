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
        using allocator_type = std::pmr::polymorphic_allocator<>;

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

        IShaderCompiler() = default;
        explicit IShaderCompiler(const allocator_type& allocator) :
            standard_entry_points(allocator),
            search_paths(allocator),
            memory_resource(allocator.resource())
        {
            standard_entry_points.emplace(ShaderStageType::kVertex, "vertexMain");
            standard_entry_points.emplace(ShaderStageType::kFragment, "fragmentMain");
            standard_entry_points.emplace(ShaderStageType::kCompute, "computeMain");
            standard_entry_points.emplace(ShaderStageType::kTessellationControl, "tesselationControlMain");
            standard_entry_points.emplace(ShaderStageType::kTessellationEval, "tesselationEvalMain");
            standard_entry_points.emplace(ShaderStageType::kGeometry, "geometryMain");
        }
        IShaderCompiler(const IShaderCompiler&) = default;
        IShaderCompiler(const IShaderCompiler& other, const allocator_type& allocator) :
            standard_entry_points(other.standard_entry_points, allocator),
            search_paths(other.search_paths, allocator),
            cache_path(other.cache_path),
            memory_resource(other.memory_resource)
        { }
        IShaderCompiler(IShaderCompiler&&) noexcept = default;
        IShaderCompiler(IShaderCompiler&& other, const allocator_type& allocator) :
            standard_entry_points(std::move(other.standard_entry_points), allocator),
            search_paths(std::move(other.search_paths), allocator),
            cache_path(std::move(other.cache_path)),
            memory_resource(other.memory_resource)
        { }

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return search_paths.get_allocator();
        }

        IShaderCompiler& operator=(const IShaderCompiler&) = default;
        IShaderCompiler& operator=(IShaderCompiler&&) noexcept = default;

        virtual ~IShaderCompiler() = default;

        virtual std::error_code compileShader(const IContext& context, ICompiledShader& compiled_shader,
                                              const std::filesystem::path& shader_path) = 0;
    };
}
