//
// Created by stuka on 16.11.2026.
//

#pragma once

#include <limits>
#include <optional>

namespace sgf::assets::utils
{
    [[nodiscard]] std::optional<std::size_t> pixelCount(
        std::uint32_t width, std::uint32_t height, std::uint32_t channels) noexcept;
}
