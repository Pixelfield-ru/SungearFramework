//
// Created by stuka on 03.11.2026.
//

#include "sgf/assets/asset_catalog.h"

#include "sgf/assets/serializer_specs.h"

sgf::assets::AssetCatalog::File const* sgf::assets::AssetCatalog::find(
    const std::filesystem::path& file_path) const noexcept
{
    const auto it = files_mapping_.find(file_path);
    if(it == files_mapping_.end()) return {};

    return &files_[it->second];
}

sgf::assets::AssetCatalog::File* sgf::assets::AssetCatalog::find(const std::filesystem::path& file_path) noexcept
{
    const auto it = files_mapping_.find(file_path);
    if(it == files_mapping_.end()) return {};

    return &files_[it->second];
}

sgf::assets::AssetCatalog::File& sgf::assets::AssetCatalog::acquireFile(std::filesystem::path file_path)
{
    files_mapping_[file_path] = files_.size();

    auto& file = files_.emplace_back();
    file.id = next_asset_id_++;
    file.path = std::move(file_path);

    return file;
}

std::error_code sgf::assets::AssetCatalog::save()
{
    if(const auto err = serializer_.serialize(*this)) return err;

    // todo: write to file!
    // serializer_.writer()->data();

    return {};
}
