//
// Created by stuka on 03.11.2026.
//

#include "sgf/assets/catalog.h"

#include "sgf/assets/serializer_specs.h"
#include "sgf/core/utility/file.h"

sgf::assets::AssetCatalog::File const* sgf::assets::AssetCatalog::findFile(
    const std::filesystem::path& file_path) const noexcept
{
    const auto it = files_mapping_.find(file_path);
    if(it == files_mapping_.end()) return {};

    return &files_[it->second];
}

sgf::assets::AssetCatalog::File* sgf::assets::AssetCatalog::findFile(const std::filesystem::path& file_path) noexcept
{
    const auto it = files_mapping_.find(file_path);
    if(it == files_mapping_.end()) return {};

    return &files_[it->second];
}

sgf::assets::Archive* sgf::assets::AssetCatalog::findArchive(const std::filesystem::path& archive_path) noexcept
{
    const auto it = archives_mapping_.find(archive_path);
    if(it == archives_mapping_.end()) return {};

    return archives_[it->second].get();
}

sgf::assets::AssetCatalog::File* sgf::assets::AssetCatalog::findFile(AssetId id) noexcept
{
    const auto it = files_mapping_ids_.find(id);
    if(it == files_mapping_ids_.end()) return {};

    return &files_[it->second];
}

sgf::assets::AssetCatalog::File const* sgf::assets::AssetCatalog::findFile(AssetId id) const noexcept
{
    const auto it = files_mapping_ids_.find(id);
    if(it == files_mapping_ids_.end()) return {};

    return &files_[it->second];
}

sgf::assets::AssetCatalog::File& sgf::assets::AssetCatalog::acquireFile(std::filesystem::path file_path)
{
    const auto file_index = files_.size();
    const auto file_id = next_asset_id_++;

    files_mapping_[file_path] = file_index;
    files_mapping_ids_[file_id] = file_index;

    auto& file = files_.emplace_back();
    file.id = file_id;
    file.path = std::move(file_path);

    return file;
}

sgf::assets::AssetCatalog::File& sgf::assets::AssetCatalog::acquireFile()
{
    const auto file_index = files_.size();
    const auto file_id = next_asset_id_++;

    files_mapping_ids_[file_id] = file_index;

    auto& file = files_.emplace_back();
    file.id = file_id;

    return file;
}

sgf::assets::Archive& sgf::assets::AssetCatalog::acquireArchive(std::filesystem::path archive_path)
{
    auto archive = core::makeUnique<Archive>(get_allocator(), path / archive_path);
    const auto archive_index = archives_.size();

    archives_.push_back(std::move(archive));
    archives_mapping_.emplace(archive_path, archive_index);

    return *archives_.back();
}

std::error_code sgf::assets::AssetCatalog::save()
{
    // finish previous document if it was not finished
    serializer_.reset();

    if(const auto err = serializer_.serialize(*this))
    {
        serializer_.reset();
        return err;
    }

    const auto content = serializer_.writer()->data();
    const std::span content_view { reinterpret_cast<const std::byte*>(content.data()), content.size() };
    const auto err = core::writeFile(path / name, content_view);

    // data() points into the writer buffer, so reset only after the file is written
    serializer_.reset();

    return err;
}

std::error_code sgf::assets::AssetCatalog::load()
{
    if(!std::filesystem::exists(path)) return std::make_error_code(std::errc::no_such_file_or_directory);

    const auto file_content = core::readFile(path / name, get_allocator());

    if(!file_content)
    {
        return file_content.error();
    }

    const std::string_view content_view { reinterpret_cast<const char*>(file_content->data()), file_content->size() };

    if(const auto err = serializer_.reader()->parse(content_view)) return err;
    if(const auto err = serializer_.deserialize(*this)) return err;

    serializer_.reset();

    return {};
}

bool sgf::assets::AssetCatalog::exists() const noexcept
{
    return std::filesystem::exists(path / name);
}
