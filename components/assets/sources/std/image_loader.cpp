//
// Created by stuka on 16.11.2026.
//

#include "sgf/assets/std/image_loader.h"

#include <limits>
#include <optional>

#include "sgf/assets/std/image_utils.h"

namespace
{
    /// Baked image: three little-endian uint32 values, then tightly packed 8-bit pixels.
    struct ImageRecord final
    {
        std::uint32_t width {};
        std::uint32_t height {};
        std::uint32_t channels {};
    };
}

std::error_code sgf::assets::ImageLoader::loadBaked(
    std::span<const std::byte> bytes,
    std::span<const AnyHandle>,
    Image& asset)
{
    if(bytes.size() < sizeof(ImageRecord)) return std::make_error_code(std::errc::result_out_of_range);

    ImageRecord record;
    std::memcpy(&record, bytes.data(), sizeof(record));

    const auto count = utils::pixelCount(record.width, record.height, record.channels);
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
    const auto count = utils::pixelCount(asset.width, asset.height, asset.channels);
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
