//
// Created by stuka on 03.11.2026.
//

#include "sgf/assets/catalog.h"

#include <mutex>

#include "sgf/assets/serializer_specs.h"
#include "sgf/core/utility/file.h"

std::optional<sgf::assets::AssetCatalog::File> sgf::assets::AssetCatalog::findFile(
    const std::filesystem::path& file_path) const
{
    std::shared_lock lock(mutex_);
    const auto it = files_mapping_.find(file_path);
    if(it == files_mapping_.end()) return {};

    return files_[it->second];
}

std::optional<sgf::assets::AssetCatalog::File> sgf::assets::AssetCatalog::findFile(AssetId id) const
{
    std::shared_lock lock(mutex_);
    const auto it = files_mapping_ids_.find(id);
    if(it == files_mapping_ids_.end()) return {};

    return files_[it->second];
}

bool sgf::assets::AssetCatalog::hasFile(AssetId id) const noexcept
{
    std::shared_lock lock(mutex_);
    const auto it = files_mapping_ids_.find(id);
    if(it == files_mapping_ids_.end()) return false;

    return true;
}

sgf::assets::Archive* sgf::assets::AssetCatalog::findArchive(const std::filesystem::path& archive_path)
{
    std::shared_lock lock(mutex_);
    const auto it = archives_mapping_.find(archive_path);
    if(it == archives_mapping_.end()) return {};

    return archives_[it->second].get();
}

sgf::assets::AssetCatalog::AcquiredFile sgf::assets::AssetCatalog::acquireFile(std::filesystem::path file_path)
{
    std::unique_lock lock(mutex_);
    const auto existing = files_mapping_.find(file_path);
    if(existing != files_mapping_.end())
    {
        return { files_[existing->second], false };
    }

    const auto file_index = files_.size();
    const auto file_id = next_asset_id_++;

    files_mapping_[file_path] = file_index;
    files_mapping_ids_[file_id] = file_index;

    auto& file = files_.emplace_back();
    file.id = file_id;
    file.path = std::move(file_path);

    return { file, true };
}

sgf::assets::AssetCatalog::File sgf::assets::AssetCatalog::acquireFile()
{
    std::unique_lock lock(mutex_);
    const auto file_index = files_.size();
    const auto file_id = next_asset_id_++;

    files_mapping_ids_[file_id] = file_index;

    auto& file = files_.emplace_back();
    file.id = file_id;

    return file;
}

sgf::assets::Archive& sgf::assets::AssetCatalog::acquireArchive(std::filesystem::path archive_path)
{
    std::unique_lock lock(mutex_);
    const auto existing = archives_mapping_.find(archive_path);
    if(existing != archives_mapping_.end())
    {
        return *archives_[existing->second];
    }

    auto archive = core::makeUnique<Archive>(get_allocator(), path_ / archive_path);
    const auto archive_index = archives_.size();

    archives_.push_back(std::move(archive));
    archives_mapping_.emplace(std::move(archive_path), archive_index);

    return *archives_.back();
}

void sgf::assets::AssetCatalog::setTypeId(AssetId id, std::int64_t type_id)
{
    std::unique_lock lock(mutex_);
    const auto it = files_mapping_ids_.find(id);
    if(it == files_mapping_ids_.end()) return;

    files_[it->second].type_id = type_id;
}

bool sgf::assets::AssetCatalog::setBaked(AssetId id, File::Baked baked)
{
    std::unique_lock lock(mutex_);
    const auto it = files_mapping_ids_.find(id);
    if(it == files_mapping_ids_.end()) return false;

    files_[it->second].baked = std::move(baked);
    return true;
}

bool sgf::assets::AssetCatalog::setBaked(const std::filesystem::path& archive, std::span<const BakedAsset> assets)
{
    std::unique_lock lock(mutex_);

    struct Update
    {
        std::size_t index {};
        File::Baked baked;
    };

    // paths are copied before any row changes, so a throw or a missing id leaves the catalog as it was
    core::Vector<Update> updates(get_allocator());
    updates.reserve(assets.size());

    // collecting all assets before to prevent the broken catalog
    for(const auto& asset : assets)
    {
        const auto it = files_mapping_ids_.find(asset.asset_id);
        if(it == files_mapping_ids_.end()) return false;

        File::Baked baked;
        baked.archive = archive;
        baked.offset = asset.offset;
        baked.size = asset.size;
        updates.push_back(Update { it->second, std::move(baked) });
    }

    for(auto& update : updates)
    {
        files_[update.index].baked = std::move(update.baked);
    }

    return true;
}

std::filesystem::path sgf::assets::AssetCatalog::root() const
{
    std::shared_lock lock(mutex_);
    return path_;
}

void sgf::assets::AssetCatalog::setRoot(std::filesystem::path root)
{
    std::unique_lock lock(mutex_);
    path_ = std::move(root);
}

std::error_code sgf::assets::AssetCatalog::save()
{
    std::unique_lock lock(mutex_);

    // finish previous document if it was not finished
    serializer_.reset();

    if(const auto err = serializer_.serialize(*this))
    {
        serializer_.reset();
        return err;
    }

    const auto content = serializer_.writer()->data();
    const std::span content_view { reinterpret_cast<const std::byte*>(content.data()), content.size() };
    const auto err = core::writeFile(path_ / name_, content_view);

    // data() points into the writer buffer, so reset only after the file is written
    serializer_.reset();

    return err;
}

std::error_code sgf::assets::AssetCatalog::load()
{
    std::unique_lock lock(mutex_);

    if(!std::filesystem::exists(path_)) return std::make_error_code(std::errc::no_such_file_or_directory);

    const auto file_content = core::readFile(path_ / name_, get_allocator());

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

bool sgf::assets::AssetCatalog::exists() const
{
    std::shared_lock lock(mutex_);
    return std::filesystem::exists(path_ / name_);
}

sgf::assets::AssetCatalog::AssetCatalog(const AssetCatalog& other) :
    name_("assets_catalog")
{
    std::shared_lock lock(other.mutex_);
    name_ = other.name_;
    next_asset_id_.store(other.next_asset_id_.load(std::memory_order_relaxed));
    path_ = other.path_;
    files_ = other.files_;
    files_mapping_ = other.files_mapping_;
    files_mapping_ids_ = other.files_mapping_ids_;
    serializer_ = other.serializer_;
}

sgf::assets::AssetCatalog::AssetCatalog(const AssetCatalog& other, const allocator_type& alloc) :
    name_("assets_catalog", alloc),
    files_(alloc),
    archives_(alloc),
    archives_mapping_(alloc),
    files_mapping_(alloc),
    files_mapping_ids_(alloc)
{
    std::shared_lock lock(other.mutex_);
    name_ = other.name_;
    next_asset_id_.store(other.next_asset_id_.load(std::memory_order_relaxed));
    path_ = other.path_;
    files_.assign(other.files_.begin(), other.files_.end());
    files_mapping_.insert(other.files_mapping_.begin(), other.files_mapping_.end());
    files_mapping_ids_.insert(other.files_mapping_ids_.begin(), other.files_mapping_ids_.end());
    serializer_ = serialization::Serializer(other.serializer_, alloc);
}

sgf::assets::AssetCatalog::AssetCatalog(AssetCatalog&& other) noexcept :
    name_("assets_catalog")
{
    std::unique_lock lock(other.mutex_);
    name_ = std::move(other.name_);
    next_asset_id_.store(other.next_asset_id_.exchange(0, std::memory_order_relaxed));
    path_ = std::move(other.path_);
    files_ = std::move(other.files_);
    archives_ = std::move(other.archives_);
    archives_mapping_ = std::move(other.archives_mapping_);
    files_mapping_ = std::move(other.files_mapping_);
    files_mapping_ids_ = std::move(other.files_mapping_ids_);
    serializer_ = std::move(other.serializer_);
}

sgf::assets::AssetCatalog::AssetCatalog(AssetCatalog&& other, const allocator_type& alloc) noexcept :
    name_("assets_catalog", alloc),
    files_(alloc),
    archives_(alloc),
    archives_mapping_(alloc),
    files_mapping_(alloc),
    files_mapping_ids_(alloc)
{
    std::unique_lock lock(other.mutex_);
    name_ = std::move(other.name_);
    next_asset_id_.store(other.next_asset_id_.exchange(0, std::memory_order_relaxed));
    path_ = std::move(other.path_);
    files_ = std::move(other.files_);
    archives_ = std::move(other.archives_);
    archives_mapping_ = std::move(other.archives_mapping_);
    files_mapping_ = std::move(other.files_mapping_);
    files_mapping_ids_ = std::move(other.files_mapping_ids_);
    serializer_ = serialization::Serializer(std::move(other.serializer_), alloc);
}

sgf::assets::AssetCatalog& sgf::assets::AssetCatalog::operator=(const AssetCatalog& other)
{
    if(this == std::addressof(other)) return *this;

    std::unique_lock lock(mutex_, std::defer_lock);
    std::shared_lock other_lock(other.mutex_, std::defer_lock);
    std::lock(lock, other_lock);

    name_ = other.name_;
    next_asset_id_.store(other.next_asset_id_.load(std::memory_order_relaxed));
    path_ = other.path_;
    files_ = other.files_;
    archives_.clear();
    archives_mapping_.clear();
    files_mapping_ = other.files_mapping_;
    files_mapping_ids_ = other.files_mapping_ids_;
    serializer_ = other.serializer_;

    return *this;
}

sgf::assets::AssetCatalog& sgf::assets::AssetCatalog::operator=(AssetCatalog&& other) noexcept
{
    if(this == std::addressof(other)) return *this;

    std::unique_lock lock(mutex_, std::defer_lock);
    std::unique_lock other_lock(other.mutex_, std::defer_lock);
    std::lock(lock, other_lock);

    name_ = std::move(other.name_);
    next_asset_id_.store(other.next_asset_id_.exchange(0, std::memory_order_relaxed));
    path_ = std::move(other.path_);
    files_ = std::move(other.files_);
    archives_ = std::move(other.archives_);
    archives_mapping_ = std::move(other.archives_mapping_);
    files_mapping_ = std::move(other.files_mapping_);
    files_mapping_ids_ = std::move(other.files_mapping_ids_);
    serializer_ = std::move(other.serializer_);

    return *this;
}
