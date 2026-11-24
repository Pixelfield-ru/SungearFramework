//
// Created by stuka on 16.11.2026.
//

#pragma once

#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <system_error>

#include "sgf/assets/std/image.h"

namespace sgf::assets::utils
{
    [[nodiscard]] std::optional<std::size_t> pixelCount(
        std::uint32_t width, std::uint32_t height, std::uint32_t channels) noexcept;

    /// Decodes 8-bit PNG or JPEG bytes into asset. Fields change only after the decode succeeds.
    [[nodiscard]] std::error_code decodeImage(std::span<const std::byte> encoded, Image& asset);
}
