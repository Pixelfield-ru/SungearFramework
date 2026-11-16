//
// Created by stuka on 16.11.2026.
//

#include "sgf/assets/std/mtl_importer.h"

#include <array>

std::span<const std::string_view> sgf::assets::MtlImporter::extensions() const
{
    static constexpr std::array<std::string_view, 1> ext { ".mtl" };
    return ext;
}

std::error_code sgf::assets::MtlImporter::importSource(const std::filesystem::path& source, Material& asset)
{
    // todo: import mtl files
    return {};
}
