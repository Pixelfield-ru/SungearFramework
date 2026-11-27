//
// Created by stuka on 16.11.2026.
//

#include "sgf/assets/std/image_utils.h"

#include <memory>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace
{
    struct StbImageDeleter
    {
        void operator()(void* pixels) const noexcept
        {
            stbi_image_free(pixels);
        }
    };
}

std::optional<std::size_t> sgf::assets::utils::pixelCount(std::uint32_t width, std::uint32_t height,
                                                          std::uint32_t channels) noexcept
{
    if(width == 0 || height == 0 || channels == 0 || channels > 4) return {};

    const auto plane = static_cast<std::uint64_t>(width) * height;
    if(plane > std::numeric_limits<std::size_t>::max() / channels) return {};

    return static_cast<std::size_t>(plane * channels);
}

std::error_code sgf::assets::utils::decodeImage(std::span<const std::byte> encoded, Image& asset)
{
    if(encoded.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
    {
        return std::make_error_code(std::errc::file_too_large);
    }

    int width = 0;
    int height = 0;
    int channels = 0;

    const std::unique_ptr<stbi_uc, StbImageDeleter> decoded(stbi_load_from_memory(
        reinterpret_cast<const stbi_uc*>(encoded.data()),
        static_cast<int>(encoded.size()),
        &width, &height, &channels, 0));

    if(!decoded) return std::make_error_code(std::errc::invalid_argument);

    const auto count = pixelCount(
        static_cast<std::uint32_t>(width),
        static_cast<std::uint32_t>(height),
        static_cast<std::uint32_t>(channels));
    if(!count) return std::make_error_code(std::errc::invalid_argument);

    const auto* begin = reinterpret_cast<const std::byte*>(decoded.get());
    core::Vector<std::byte> pixels(begin, begin + *count, asset.pixels.get_allocator());

    asset.width = static_cast<std::uint32_t>(width);
    asset.height = static_cast<std::uint32_t>(height);
    asset.channels = static_cast<std::uint32_t>(channels);
    asset.pixels = std::move(pixels);

    return {};
}
