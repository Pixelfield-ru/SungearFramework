//
// Created by stuka on 18.11.2026.
//

#include "sgf/assets/std/model_loader.h"

std::error_code sgf::assets::ModelLoader::loadBaked(std::span<const std::byte> bytes,
                                                    std::span<const AnyHandle> dependencies, Model& asset)
{
    return {};
}

std::expected<sgf::core::Vector<std::byte>, std::error_code> sgf::assets::ModelLoader::saveBaked(const Model& asset)
{
    return {};
}

std::error_code sgf::assets::ModelLoader::loadMeta(const std::filesystem::path& source, AssetId id)
{
    return {};
}

std::error_code sgf::assets::ModelLoader::writeMeta(const std::filesystem::path& source, AssetId id)
{
    return {};
}

sgf::core::Vector<sgf::assets::AnyHandle> sgf::assets::ModelLoader::dependencies(const Model& asset)
{
    return {};
}
