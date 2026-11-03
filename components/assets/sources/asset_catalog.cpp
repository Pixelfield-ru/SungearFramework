//
// Created by stuka on 03.11.2026.
//

#include "sgf/assets/asset_catalog.h"

#include "sgf/assets/serializer_specs.h"
#include "sgf/core/utility/file.h"

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

sgf::assets::AssetId sgf::assets::AssetCatalog::acquireId()
{
    return next_asset_id_++;
}

std::error_code sgf::assets::AssetCatalog::save()
{
    if(const auto err = serializer_.serialize(*this)) return err;

    const auto content = serializer_.writer()->data();

    std::span content_view { reinterpret_cast<const std::byte*>(content.data()), content.size() };

    return core::writeFile(path, content_view);
}

std::error_code sgf::assets::AssetCatalog::load()
{
    if(!std::filesystem::exists(path)) return std::make_error_code(std::errc::no_such_file_or_directory);

    const auto file_content = core::readFile(path, get_allocator());

    if(!file_content)
    {
        return file_content.error();
    }

    const std::string_view content_view { reinterpret_cast<const char*>(file_content->data()), file_content->size() };

    if(const auto err = serializer_.reader()->parse(content_view)) return err;
    if(const auto err = serializer_.deserialize(*this)) return err;

    return {};
}
