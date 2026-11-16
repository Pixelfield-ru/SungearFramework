//
// Created by stuka on 16.11.2026.
//

#include "sgf/assets/std/material_loader.h"

std::error_code sgf::assets::MaterialLoader::loadBaked(std::span<const std::byte> bytes,
    std::span<const AnyHandle> dependencies, Material& asset)
{
    return {};
}

std::expected<sgf::core::Vector<std::byte>, std::error_code> sgf::assets::MaterialLoader::saveBaked(
    const Material& asset)
{
    return {};
}

std::error_code sgf::assets::MaterialLoader::loadMeta(const std::filesystem::path& source, AssetId id)
{
    return {};
}

std::error_code sgf::assets::MaterialLoader::writeMeta(const std::filesystem::path& source, AssetId id)
{
    return {};
}

sgf::core::Vector<sgf::assets::AnyHandle> sgf::assets::MaterialLoader::dependencies(const Material& asset)
{
    return {};
}
