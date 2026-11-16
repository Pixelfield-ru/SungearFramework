//
// Created by stuka on 12.10.2026.
//

#pragma once

#include <span>
#include <string>
#include <filesystem>

#include "sgf/core/memory.h"

namespace sgf::core
{
    [[nodiscard]] String toUtf8(char32_t codepoint, const std::pmr::polymorphic_allocator<>& alloc = {});
    [[nodiscard]] String toUtf8(std::span<const char32_t> codepoints, const std::pmr::polymorphic_allocator<>& alloc = {});

    String pathToUtf8(const std::filesystem::path& p) noexcept;
}