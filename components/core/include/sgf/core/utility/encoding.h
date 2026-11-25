//
// Created by stuka on 12.10.2026.
//

#pragma once

#include <expected>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <filesystem>

#include "sgf/core/memory.h"

namespace sgf::core
{
    [[nodiscard]] String toUtf8(char32_t codepoint, const std::pmr::polymorphic_allocator<>& alloc = {});
    [[nodiscard]] String toUtf8(std::span<const char32_t> codepoints, const std::pmr::polymorphic_allocator<>& alloc = {});
    /// A truncated sequence, an overlong form, or a surrogate is an error.
    [[nodiscard]] std::expected<String, std::error_code> toUtf8(
        std::u16string_view text, const std::pmr::polymorphic_allocator<>& alloc = {});

    /// A truncated sequence, an overlong form, or a surrogate is an error.
    [[nodiscard]] std::expected<BasicString<char16_t>, std::error_code> toUtf16(
        std::string_view text, const std::pmr::polymorphic_allocator<>& alloc = {});

    String pathToUtf8(const std::filesystem::path& p, const std::pmr::polymorphic_allocator<>& alloc = { }) noexcept;
}