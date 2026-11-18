//
// Created by stuka on 18.11.2026.
//

#include "sgf/assets/std/obj_importer.h"

#include <array>

std::span<const std::string_view> sgf::assets::ObjImporter::extensions() const
{
    static constexpr std::array<std::string_view, 1> ext { ".obj" };
    return ext;
}

std::error_code sgf::assets::ObjImporter::importSource(const std::filesystem::path& source, Model& asset)
{
    // todo: impl
    return {};
}
