//
// Created by stuka on 16.11.2026.
//

#include "sgf/assets/std/png_importer.h"

#include <array>

#include "sgf/core/utility/file.h"
#include "sgf/assets/std/image_utils.h"

std::span<const std::string_view> sgf::assets::PngImporter::extensions() const
{
    static constexpr std::array<std::string_view, 3> ext { ".png", ".jpg", ".jpeg" };
    return ext;
}

std::error_code sgf::assets::PngImporter::importSource(const std::filesystem::path& source, Image& asset, AssetServer&)
{
    auto file = core::readFile(source, asset.pixels.get_allocator());
    if(!file) return file.error();

    return utils::decodeImage({ file->data(), file->size() }, asset);
}
