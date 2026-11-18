//
// Created by stuka on 18.11.2026.
//

#include "sgf/assets/std/stl_importer.h"

#include <array>

std::span<const std::string_view> sgf::assets::StlImporter::extensions() const
{
    static constexpr std::array<std::string_view, 1> ext { ".stl" };
    return ext;
}

std::error_code sgf::assets::StlImporter::importSource(const std::filesystem::path& source, Model& asset)
{
    // todo: impl
    return {};
}
