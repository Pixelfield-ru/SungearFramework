//
// Created by stuka on 16.11.2026.
//

#include "sgf/assets/std/image_loader.h"

#include "sgf/core/utility/file.h"

#include <cstring>
#include <limits>
#include <memory>
#include <optional>

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

    /// Baked image: three little-endian uint32 values, then tightly packed 8-bit pixels.
    struct ImageRecord final
    {
        std::uint32_t width {};
        std::uint32_t height {};
        std::uint32_t channels {};
    };

    [[nodiscard]] std::optional<std::size_t> pixelCount(
        std::uint32_t width, std::uint32_t height, std::uint32_t channels) noexcept
    {
        if(width == 0 || height == 0 || channels == 0 || channels > 4) return {};

        const auto plane = static_cast<std::uint64_t>(width) * height;
        if(plane > std::numeric_limits<std::size_t>::max() / channels) return {};

        return static_cast<std::size_t>(plane * channels);
    }
}

std::error_code sgf::assets::ImageLoader::loadSource(const std::filesystem::path& source, Image& asset)
{
    auto file = core::readFile(source, asset.pixels.get_allocator());
    if(!file) return file.error();

    if(file->size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
    {
        return std::make_error_code(std::errc::file_too_large);
    }

    int width = 0;
    int height = 0;
    int channels = 0;

    const std::unique_ptr<stbi_uc, StbImageDeleter> decoded(stbi_load_from_memory(
        reinterpret_cast<const stbi_uc*>(file->data()),
        static_cast<int>(file->size()),
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

std::error_code sgf::assets::ImageLoader::loadBaked(
    std::span<const std::byte> bytes,
    std::span<const AnyHandle>,
    Image& asset)
{
    if(bytes.size() < sizeof(ImageRecord)) return std::make_error_code(std::errc::result_out_of_range);

    ImageRecord record;
    std::memcpy(&record, bytes.data(), sizeof(record));

    const auto count = pixelCount(record.width, record.height, record.channels);
    if(!count || bytes.size() != sizeof(ImageRecord) + *count)
    {
        return std::make_error_code(std::errc::result_out_of_range);
    }

    const auto pixels_begin = bytes.begin() + static_cast<std::ptrdiff_t>(sizeof(ImageRecord));
    core::Vector<std::byte> pixels(pixels_begin, bytes.end(), asset.pixels.get_allocator());

    asset.width = record.width;
    asset.height = record.height;
    asset.channels = record.channels;
    asset.pixels = std::move(pixels);

    return {};
}

std::expected<sgf::core::Vector<std::byte>, std::error_code> sgf::assets::ImageLoader::saveBaked(const Image& asset)
{
    const auto count = pixelCount(asset.width, asset.height, asset.channels);
    if(!count || asset.pixels.size() != *count)
    {
        return std::unexpected(std::make_error_code(std::errc::invalid_argument));
    }

    core::Vector<std::byte> bytes(sizeof(ImageRecord) + *count, asset.pixels.get_allocator());

    const ImageRecord record { asset.width, asset.height, asset.channels };
    std::memcpy(bytes.data(), &record, sizeof(record));
    std::memcpy(bytes.data() + sizeof(record), asset.pixels.data(), asset.pixels.size());

    return bytes;
}

std::error_code sgf::assets::ImageLoader::loadMeta(const std::filesystem::path&, AssetId)
{
    return {};
}

std::error_code sgf::assets::ImageLoader::writeMeta(const std::filesystem::path&, AssetId)
{
    return {};
}

sgf::core::Vector<sgf::assets::AnyHandle> sgf::assets::ImageLoader::dependencies(const Image&)
{
    return {};
}
