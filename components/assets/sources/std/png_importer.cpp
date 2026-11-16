//
// Created by stuka on 16.11.2026.
//

#include "sgf/assets/std/png_importer.h"

#include <array>

#include "sgf/core/utility/file.h"
#include "sgf/assets/std/image_utils.h"

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

std::span<const std::string_view> sgf::assets::PngImporter::extensions() const
{
    static constexpr std::array<std::string_view, 3> ext { ".png", ".jpg", ".jpeg" };
    return ext;
}

std::error_code sgf::assets::PngImporter::importSource(const std::filesystem::path& source, Image& asset)
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

    const auto count = utils::pixelCount(
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
