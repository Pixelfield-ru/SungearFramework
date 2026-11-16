//
// Created by stuka on 16.11.2026.
//

#include "sgf/assets/std/image_utils.h"

std::optional<std::size_t> sgf::assets::utils::pixelCount(std::uint32_t width, std::uint32_t height,
                                                          std::uint32_t channels) noexcept
{
    if(width == 0 || height == 0 || channels == 0 || channels > 4) return {};

    const auto plane = static_cast<std::uint64_t>(width) * height;
    if(plane > std::numeric_limits<std::size_t>::max() / channels) return {};

    return static_cast<std::size_t>(plane * channels);
}
