//
// Created by stuka on 03.09.2026.
//

#pragma once

#include <string>

#include "sgf/core/memory.h"
#include "types.h"

namespace sgf::rhi
{
    struct ShaderStage
    {
        using allocator_type = std::pmr::polymorphic_allocator<>;

        ShaderStageType type = ShaderStageType::kUnknown;
        core::String entry_point_name;

        ShaderStage() = default;
        explicit ShaderStage(const allocator_type& allocator) :
            entry_point_name(allocator)
        { }
        ShaderStage(const ShaderStage&) = default;
        ShaderStage(const ShaderStage& other, const allocator_type& allocator) :
            type(other.type),
            entry_point_name(other.entry_point_name, allocator)
        { }
        ShaderStage(ShaderStage&&) noexcept = default;
        ShaderStage(ShaderStage&& other, const allocator_type& allocator) :
            type(other.type),
            entry_point_name(std::move(other.entry_point_name), allocator)
        { }

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return entry_point_name.get_allocator();
        }

        ShaderStage& operator=(const ShaderStage&) = default;
        ShaderStage& operator=(ShaderStage&&) noexcept = default;
    };
}