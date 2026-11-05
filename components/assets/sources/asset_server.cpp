//
// Created by stuka on 28.10.2026.
//

#include "sgf/assets/asset_server.h"

std::error_code sgf::assets::AssetServer::mount(std::filesystem::path path) noexcept
{
    catalog_.path = std::move(path);

    if(!catalog_.exists())
    {
        return catalog_.save();
    }

    return catalog_.load();
}

std::expected<sgf::assets::AnyHandle, std::error_code> sgf::assets::AssetServer::load(
    AssetId asset_id, std::uint32_t type_id) noexcept
{
    auto* storage = this->storage(type_id);
    if(!storage)
    {
        return std::unexpected(Error::kInvalidStorage);
    }

    auto* loader = storage->erasedLoader();
    if(!loader)
    {
        return std::unexpected(Error::kLoaderNotSpecified);
    }

    auto* slot = storage->slotControl(asset_id);
    // if slot is already exists then leave
    if(slot && (slot->loaded() || slot->state == AssetState::kLoading))
    {
        return AnyHandle(slot, type_id);
    }

    // trying to find existing asset
    auto* file = catalog_.findFile(asset_id);

    // file does not exist in catalog
    if(!file)
    {
        return std::unexpected(Error::kUnknownAsset);
    }

    // else if slot does not exist then acquire new slot
    if(!slot) slot = storage->acquireErasedSlot(asset_id);

    // if files is stored by path
    if(file->path)
    {
        const auto actual_path = catalog_.path / *file->path;

        // loading meta of the file near the file
        if(const auto err = loader->loadMeta(actual_path, file->id)) return std::unexpected(err);

        slot->state = AssetState::kLoading;
        // loading asset by path
        // todo: async
        if(const auto err = loader->loadSourceErased(actual_path, slot->erased()))
        {
            slot->state = AssetState::kFailed;
            return std::unexpected(err);
        }

        slot->state = AssetState::kLoaded;

        return AnyHandle(slot, type_id);
    }

    if(file->baked)
    {
        // loading asset from archive
        auto* archive = catalog_.findArchive(file->baked->archive);
        if(!archive)
        {
            archive = &catalog_.acquireArchive(file->baked->archive);
        }

        if(const auto err = archive->open(std::ios::in)) return std::unexpected(err);

        const auto archive_bytes = archive->bytes();

        const auto baked_offset = file->baked->offset;
        const auto baked_size = file->baked->size;

        // baked_offset + baked_size may provide integer overflow
        if(baked_offset > archive_bytes.size() ||
           baked_size > archive_bytes.size() - baked_offset)
        {
            return std::unexpected(std::make_error_code(std::errc::result_out_of_range));
        }

        const auto asset_bytes = archive_bytes.subspan(baked_offset, baked_size);

        slot->state = AssetState::kLoading;
        // todo: async
        if(const auto err = loader->loadBakedErased(asset_bytes, slot->erased()))
        {
            slot->state = AssetState::kFailed;
            return std::unexpected(err);
        }

        slot->state = AssetState::kLoaded;

        return AnyHandle(slot, type_id);
    }

    return std::unexpected(Error::kUnknownStorageType);
}

std::error_code sgf::assets::AssetServer::reload(const AnyHandle& handle) noexcept
{
    const auto handle_valid = handle.slot_ && handle.slot_->erased();
    if(!handle_valid) return Error::kInvalidHandle;

    if(handle.slot_->state == AssetState::kLoading) return {};

    auto* storage = this->storage(handle.type_id_);
    if(!storage)
    {
        return Error::kInvalidStorage;
    }

    auto* loader = storage->erasedLoader();
    if(!loader)
    {
        return Error::kLoaderNotSpecified;
    }

    // trying to find existing asset
    auto* file = catalog_.findFile(handle.asset_id_);

    if(!file)
    {
        return Error::kUnknownAsset;
    }
    // file was baked but not saved in file. no error
    if(!file->path)
    {
        return Error::kNoSource;
    }

    const auto file_path = catalog_.path / *file->path;

    // write meta before loading source
    if(const auto err = loader->writeMeta(file_path, handle.asset_id_)) return err;

    handle.slot_->state = AssetState::kLoading;
    // finally, load source
    // todo: async
    if(const auto err = storage->erasedLoader()->loadSourceErased(file_path, handle.slot_->erased()))
    {
        handle.slot_->state = AssetState::kFailed;
        return err;
    }

    // todo: async!!!
    ++handle.slot_->version;
    handle.slot_->state = AssetState::kLoaded;

    return {};
}
