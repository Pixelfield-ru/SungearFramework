//
// Created by stuka on 28.10.2026.
//

#include "sgf/assets/server.h"

#include <limits>

#include <zstd.h>

#include "sgf/core/errors.h"

void sgf::assets::AssetServer::finishPosted(const core::SharedPtr<PostedJobs>& posted) noexcept
{
    if(!posted) return;

    if(posted->count.fetch_sub(1, std::memory_order_acq_rel) == 1)
    {
        posted->count.notify_all();
    }
}

sgf::assets::AssetServer::~AssetServer()
{
    // waiting for all tasks
    waitPosted();
}

void sgf::assets::AssetServer::waitPosted() noexcept
{
    if(!posted_) return;

    auto pending = posted_->count.load(std::memory_order_acquire);
    while(pending != 0)
    {
        posted_->count.wait(pending, std::memory_order_acquire);
        pending = posted_->count.load(std::memory_order_acquire);
    }
}

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
    auto* storage = this->storage(core::typeIndexFromStableHash(type_id));
    if(!storage)
    {
        return std::unexpected(Error::kInvalidStorage);
    }

    auto* loader = storage->erasedLoader();
    if(!loader)
    {
        return std::unexpected(Error::kLoaderNotSpecified);
    }

    auto* slot = storage->retainErasedSlot(asset_id);
    // if slot is already exists then leave
    if(slot && (slot->loaded() || slot->state == AssetState::kLoading))
    {
        AnyHandle handle;
        handle.slot_ = slot;
        handle.asset_id_ = asset_id;
        handle.type_id_ = type_id;
        return handle;
    }

    // trying to find existing asset
    auto* file = catalog_.findFile(asset_id);

    // file does not exist in catalog
    if(!file)
    {
        if(slot) slot->decrementRefCounter();
        return std::unexpected(Error::kUnknownAsset);
    }

    // retainErasedSlot already owns one ref. A new slot starts at zero refs.
    const bool retained = slot != nullptr;
    if(!slot) slot = storage->acquireErasedSlot(asset_id);

    AnyHandle handle;
    if(retained)
    {
        handle.slot_ = slot;
        handle.asset_id_ = asset_id;
        handle.type_id_ = type_id;
    }
    else
    {
        handle = AnyHandle(slot, type_id);
    }

    // if files is stored by path
    if(file->path && std::filesystem::exists(catalog_.path / *file->path))
    {
        const auto actual_path = catalog_.path / *file->path;

        // loading meta of the file near the file
        if(const auto err = loader->loadMeta(actual_path, file->id)) return std::unexpected(err);

        slot->state = AssetState::kLoading;

        // loading asset by path
        // capture the handle to increase lifetime of the slot
        if(const auto err = postTracked([loader, actual_path, handle] {
            auto* slot = handle.slot_;

            if(const auto err = loader->loadSourceErased(actual_path, slot->erased()))
            {
                slot->error = err;
                slot->state = AssetState::kFailed;
                return;
            }

            slot->error.clear();
            slot->state = AssetState::kLoaded;
        }))
        {
            slot->error = err;
            slot->state = AssetState::kFailed;
            return std::unexpected(err);
        }

        return handle;
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
        const auto allocator = get_allocator();

        slot->state = AssetState::kLoading;

        // loading baked source
        // capture the handle to increase lifetime of the slot
        if(const auto err = postTracked([loader, asset_bytes, handle, allocator] {
            auto* slot = handle.slot_;

            std::span<const std::byte> plain = asset_bytes;
            // owned by this task. do not store reference on it
            core::Vector<std::byte> decoded(allocator);

            // missing frame means the payload was stored raw. compression did not shrink it
            const auto raw_size = ZSTD_getFrameContentSize(asset_bytes.data(), asset_bytes.size());
            if(raw_size != ZSTD_CONTENTSIZE_ERROR && raw_size != ZSTD_CONTENTSIZE_UNKNOWN)
            {
                if(raw_size > std::numeric_limits<std::size_t>::max())
                {
                    slot->error = Error::kCompressionFailed;
                    slot->state = AssetState::kFailed;
                    return;
                }

                decoded.resize(static_cast<std::size_t>(raw_size));

                const auto written = ZSTD_decompress(
                    decoded.data(), decoded.size(),
                    asset_bytes.data(), asset_bytes.size());
                if(ZSTD_isError(written))
                {
                    slot->error = Error::kCompressionFailed;
                    slot->state = AssetState::kFailed;
                    return;
                }

                plain = { decoded.data(), written };
            }

            if(const auto err = loader->loadBakedErased(plain, slot->erased()))
            {
                slot->error = err;
                slot->state = AssetState::kFailed;
                return;
            }

            slot->error.clear();
            slot->state = AssetState::kLoaded;
        }))
        {
            slot->error = err;
            slot->state = AssetState::kFailed;
            return std::unexpected(err);
        }

        return handle;
    }

    return std::unexpected(Error::kUnknownStorageType);
}

std::error_code sgf::assets::AssetServer::reload(const AnyHandle& handle) noexcept
{
    auto* slot = handle.slot_;

    const auto handle_valid = slot && slot->erased();
    if(!handle_valid) return Error::kInvalidHandle;

    if(slot->state == AssetState::kLoading) return {};

    auto* storage = this->storage(core::typeIndexFromStableHash(handle.type_id_));
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

    slot->state = AssetState::kLoading;

    // finally, load source
    // capture the handle to increase lifetime of the slot
    if(const auto err = postTracked([loader, file_path, handle] {
        auto* slot = handle.slot_;

        if(const auto err = loader->loadSourceErased(file_path, slot->erased()))
        {
            slot->error = err;
            slot->state = AssetState::kFailed;
            return;
        }

        slot->error.clear();
        ++slot->version;
        slot->state = AssetState::kLoaded;
    }))
    {
        slot->error = err;
        slot->state = AssetState::kFailed;
        return err;
    }

    return {};
}

std::error_code sgf::assets::AssetServer::bake(
    const AnyHandle& handle, std::filesystem::path archive, std::int32_t compression_level)
{
    if(compression_level < ZSTD_minCLevel() || compression_level > ZSTD_maxCLevel())
    {
        return Error::kCompressionFailed;
    }

    auto archive_path = core::normalizedRelativePath(catalog_.path / archive, catalog_.path);
    if(!archive_path) return archive_path.error();

    BakeContext context;
    context.output_bytes = core::Vector<std::byte>(get_allocator());
    context.baked_assets = core::UnorderedSet<AssetId>(get_allocator());
    context.archive_path = std::move(*archive_path);
    context.compression_level = compression_level;

    if(const auto err = bakeImpl(context, handle))
    {
        return err;
    }

    if(const auto err = core::writeFile(catalog_.path / context.archive_path, context.output_bytes))
    {
        return err;
    }

    return catalog_.save();
}

std::error_code sgf::assets::AssetServer::postTracked(std::move_only_function<void()> task) noexcept
{
    if(!thread_pool_ || !posted_) return core::Error::kUninitialized;

    posted_->count.fetch_add(1, std::memory_order_acq_rel);
    try
    {
        auto posted = posted_;
        thread_pool_->post([posted, task = std::move(task)]() mutable {
            try
            {
                task();
            }
            catch (...)
            {
                finishPosted(posted);
                throw;
            }

            finishPosted(posted);
        });
    }
    catch(...)
    {
        finishPosted(posted_);
        return std::make_error_code(std::errc::resource_unavailable_try_again);
    }

    return {};
}

std::error_code sgf::assets::AssetServer::bakeImpl(BakeContext& context, const AnyHandle& handle)
{
    // already was baked
    if(context.baked_assets.contains(handle.asset_id_)) return {};
    if(handle.asset_id_ == 0 || handle.type_id_ == 0) return {};

    auto* storage = this->storage(core::typeIndexFromStableHash(handle.type_id_));
    if(!storage)
    {
        return Error::kInvalidStorage;
    }

    auto* slot = handle.slot_;
    if(!slot) slot = storage->slotControl(handle.asset_id_);

    const auto handle_valid = slot && slot->erased();
    if(!handle_valid) return Error::kInvalidHandle;

    if(!slot->loaded()) return Error::kNotLoaded;

    auto* loader = storage->erasedLoader();
    if(!loader)
    {
        return Error::kLoaderNotSpecified;
    }

    const auto bake_result = loader->saveBakedErased(slot->erased());
    if(!bake_result)
    {
        return bake_result.error();
    }

    const auto& plain = bake_result->bytes;

    const auto bound = ZSTD_compressBound(plain.size());
    if(ZSTD_isError(bound)) return Error::kCompressionFailed;

    core::Vector<std::byte> compressed(bound, get_allocator());
    const auto written = ZSTD_compress(
        compressed.data(), compressed.size(),
        plain.data(), plain.size(),
        context.compression_level);
    if(ZSTD_isError(written)) return Error::kCompressionFailed;

    // a zstd frame can be larger than the raw payload. store the raw bytes in that case
    std::span<const std::byte> stored = plain;
    if(written < plain.size())
    {
        compressed.resize(written);
        stored = { compressed.data(), compressed.size() };
    }

    AssetCatalog::File::Baked baked;
    baked.archive = context.archive_path;
    baked.offset = context.output_bytes.size();
    baked.size = stored.size();

    auto* file = catalog_.findFile(handle.asset_id_);
    file->baked = std::move(baked);

    context.output_bytes.reserve(context.output_bytes.size() + stored.size());
    context.output_bytes.insert(context.output_bytes.end(), stored.begin(), stored.end());
    context.baked_assets.insert(handle.asset_id_);

    // baking all direct dependencies
    for(const auto& dependency : bake_result->dependencies)
    {
        if(const auto err = bakeImpl(context, dependency))
        {
            return err;
        }
    }

    return {};
}

std::error_code sgf::assets::AssetServer::create(AssetServerCreateInfo create_info, AssetServer& output) noexcept
{
    if(create_info.page_size == 0) return core::Error::kInvalidArgument;
    if(!create_info.thread_pool) return core::Error::kInvalidArgument;

    // waiting for old tasks
    output.waitPosted();

    if(!output.posted_) output.posted_ = core::makeShared<PostedJobs>(output.get_allocator());

    output.catalog_ = AssetCatalog(std::move(create_info.serializer), output.get_allocator());
    output.page_size_ = create_info.page_size;
    output.debug_output_ = create_info.debug_output;
    output.thread_pool_ = create_info.thread_pool;

    return {};
}

sgf::assets::AssetServer& sgf::assets::AssetServer::operator=(AssetServer&& other) noexcept
{
    if(this == &other) return *this;

    // waiting for old tasks
    waitPosted();

    storages_map_ = std::move(other.storages_map_);
    catalog_ = std::move(other.catalog_);
    page_size_ = other.page_size_;
    debug_output_ = other.debug_output_;
    thread_pool_ = other.thread_pool_;
    posted_ = std::move(other.posted_);
    return *this;
}
