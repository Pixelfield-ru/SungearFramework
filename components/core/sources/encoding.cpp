//
// Created by stuka on 12.10.2026.
//

#include "sgf/core/utility/encoding.h"

sgf::core::String sgf::core::pathToUtf8(const std::filesystem::path& path) noexcept
{
    const auto u8 = path.u8string();
    return { u8.begin(), u8.end() };
}
