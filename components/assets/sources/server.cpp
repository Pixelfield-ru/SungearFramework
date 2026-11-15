//
// Created by stuka on 28.10.2026.
//

#include "sgf/assets/server.h"

#include "sgf/assets/header.h"

#include <limits>
#include <mutex>

#include <zstd.h>

#include "sgf/core/errors.h"

sgf::assets::AssetServer::AssetServer(AssetServer&& other) noexcept
{
    std::unique_lock lock(other.storages_mutex_);
    storages_map_ = std::move(other.storages_map_);
    catalog_ = std::move(other.catalog_);
    page_size_ = other.page_size_;
    debug_output_ = other.debug_output_;
    thread_pool_ = other.thread_pool_;
    posted_ = std::move(other.posted_);
}

sgf::assets::AssetServer::AssetServer(AssetServer&& other, const allocator_type& alloc) noexcept :
    catalog_(alloc),
    posted_(core::makeShared<PostedJobs>(alloc))
{
    std::unique_lock lock(other.storages_mutex_);
    storages_map_ = core::TypeMap(std::move(other.storages_map_), alloc);
    catalog_ = AssetCatalog(std::move(other.catalog_), alloc);
    page_size_ = other.page_size_;
    debug_output_ = other.debug_output_;
    thread_pool_ = other.thread_pool_;
    posted_ = std::move(other.posted_);
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
    catalog_.setRoot(std::move(path));

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
        core::LogCallback::error(debug_output_,
                                 "Cannot find storage. Asset ID: {}, type ID: {}",
                                 asset_id, type_id);
        return std::unexpected(Error::kInvalidStorage);
    }

    auto* loader = storage->erasedLoader();
    if(!loader)
    {
        core::LogCallback::error(debug_output_,
                                 "Cannot find loader. Asset ID: {}, type ID: {}",
                                 asset_id, type_id);
        return std::unexpected(Error::kLoaderNotSpecified);
    }

    // trying to find existing asset
    const auto file = catalog_.findFile(asset_id);

    // file does not exist in catalog
    if(!file)
    {
        core::LogCallback::error(debug_output_,
                                 "Cannot find file in catalog. Asset ID: {}, type ID: {}",
                                 asset_id, type_id);
        return std::unexpected(Error::kUnknownAsset);
    }

    const auto root = catalog_.root();

    // one lock finds or creates the slot and publishes kLoading before anyone else can acquire it
    const auto claimed = storage->claimErasedSlot(asset_id);
    auto* slot = claimed.slot;

    AnyHandle handle;
    handle.slot_ = slot;
    handle.asset_id_ = asset_id;
    handle.type_id_ = type_id;

    // the slot is already kLoading by other caller
    if(!claimed.load) return handle;

    // if files is stored by path
    if(file->path && std::filesystem::exists(root / *file->path))
    {
        const auto actual_path = root / *file->path;

        // loading meta of the file near the file
        if(const auto err = loader->loadMeta(actual_path, file->id))
        {
            slot->error = err;
            slot->state = AssetState::kFailed;
            return std::unexpected(err);
        }

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
        // the archive stays open for this load. mount and close are not called while it runs
        auto& archive = catalog_.acquireArchive(file->baked->archive);
        if(const auto err = archive.open(std::ios::in))
        {
            slot->error = err;
            slot->state = AssetState::kFailed;
            return std::unexpected(err);
        }

        const auto archive_path = archive.path().generic_string();
        const auto archive_span = archive.bytes();

        const auto baked_offset = file->baked->offset;
        const auto baked_size = file->baked->size;

        // baked_offset + baked_size may provide integer overflow
        if(baked_offset > archive_span.size() ||
           baked_size > archive_span.size() - baked_offset)
        {
            core::LogCallback::error(debug_output_,
                                     "Invalid baked file. Archive: '{}', asset ID: {}, type ID: {}",
                                     archive_path,
                                     asset_id, type_id);
            slot->error = std::make_error_code(std::errc::result_out_of_range);
            slot->state = AssetState::kFailed;
            return std::unexpected(std::make_error_code(std::errc::result_out_of_range));
        }

        const auto asset_record_bytes = archive_span.subspan(baked_offset, baked_size);

        if(asset_record_bytes.size() < sizeof(AssetHeader))
        {
            core::LogCallback::error(debug_output_,
                                     "No bytes for asset. Archive: '{}', asset ID: {}, type ID: {}",
                                     archive_path,
                                     asset_id, type_id);
            slot->error = std::make_error_code(std::errc::result_out_of_range);
            slot->state = AssetState::kFailed;
            return std::unexpected(std::make_error_code(std::errc::result_out_of_range));
        }

        AssetHeader header;
        // reading header
        std::memcpy(&header, asset_record_bytes.data(), sizeof(AssetHeader));

        const auto after_header = asset_record_bytes.size() - sizeof(AssetHeader);
        if(header.content_block_size > after_header ||
           header.dependencies_block_size > after_header - header.content_block_size)
        {
            core::LogCallback::error(debug_output_,
                                     "Invalid header of asset. Archive: '{}', asset ID: {}, type ID: {}",
                                     archive_path,
                                     asset_id, type_id);
            slot->error = std::make_error_code(std::errc::result_out_of_range);
            slot->state = AssetState::kFailed;
            return std::unexpected(std::make_error_code(std::errc::result_out_of_range));
        }

        // each stored handle is asset id, then type id
        constexpr auto handle_stride = sizeof(std::uint32_t) * 2;
        if(header.dependencies_block_size % handle_stride != 0)
        {
            core::LogCallback::error(debug_output_,
                                     "Invalid dependencies block. Archive: '{}', asset ID: {}, type ID: {}",
                                     archive_path,
                                     asset_id, type_id);
            slot->error = std::make_error_code(std::errc::result_out_of_range);
            slot->state = AssetState::kFailed;
            return std::unexpected(std::make_error_code(std::errc::result_out_of_range));
        }

        const auto asset_bytes = asset_record_bytes.subspan(sizeof(AssetHeader),
                                                            static_cast<std::size_t>(header.content_block_size));
        const auto dependency_bytes = asset_record_bytes.subspan(
            sizeof(AssetHeader) + static_cast<std::size_t>(header.content_block_size),
            static_cast<std::size_t>(header.dependencies_block_size));

        const auto allocator = get_allocator();

        core::Vector<AnyHandle> dependencies(allocator);
        const auto dependency_count = header.dependencies_block_size / handle_stride;
        dependencies.reserve(dependency_count);

        // collect & load dependencies
        for(std::size_t i = 0; i < dependency_count; ++i)
        {
            std::uint32_t dependency_id = 0;
            std::uint32_t dependency_type = 0;

            const auto* entry = dependency_bytes.data() + i * handle_stride;
            std::memcpy(&dependency_id, entry, sizeof(dependency_id));
            std::memcpy(&dependency_type, entry + sizeof(dependency_id), sizeof(dependency_type));

            // a zero id keeps the field index and stays an empty handle
            if(dependency_id == 0)
            {
                dependencies.emplace_back();
                continue;
            }

            // load dependency
            auto loaded = load(dependency_id, dependency_type);
            if(!loaded)
            {
                slot->error = loaded.error();
                slot->state = AssetState::kFailed;
                return std::unexpected(loaded.error());
            }

            dependencies.push_back(std::move(*loaded));
        }

        // loading baked source
        // capture the handle to increase lifetime of the slot
        if(const auto err = postTracked([loader, asset_bytes, handle, allocator, dependencies = std::move(dependencies)] {
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

            if(const auto err = loader->loadBakedErased(plain, dependencies, slot->erased()))
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

    core::LogCallback::error(debug_output_,
                             "The asset was saved using unknown way. Asset ID: {}, type ID: {}",
                             asset_id, type_id);

    slot->error = Error::kUnknownStorageType;
    slot->state = AssetState::kFailed;
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
        core::LogCallback::error(debug_output_,
                                 "Cannot find storage. Asset ID: {}, type ID: {}",
                                 handle.asset_id_, handle.type_id_);
        return Error::kInvalidStorage;
    }

    auto* loader = storage->erasedLoader();
    if(!loader)
    {
        core::LogCallback::error(debug_output_,
                                 "Cannot find loader. Asset ID: {}, type ID: {}",
                                 handle.asset_id_, handle.type_id_);
        return Error::kLoaderNotSpecified;
    }

    // trying to find existing asset
    const auto file = catalog_.findFile(handle.asset_id_);

    if(!file)
    {
        core::LogCallback::error(debug_output_,
                                 "Cannot find file in catalog. Asset ID: {}, type ID: {}",
                                 handle.asset_id_, handle.type_id_);
        return Error::kUnknownAsset;
    }

    const auto root = catalog_.root();

    // asset was not saved in file
    if(!file->path || !std::filesystem::exists(root / *file->path))
    {
        core::LogCallback::error(debug_output_,
                                 "Cannot find file in filesystem. Asset ID: {}, type ID: {}",
                                 handle.asset_id_, handle.type_id_);
        return Error::kNoSource;
    }

    const auto file_path = root / *file->path;

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
    const AnyHandle& handle, const std::filesystem::path& archive, std::int32_t compression_level)
{
    if(compression_level < ZSTD_minCLevel() || compression_level > ZSTD_maxCLevel())
    {
        core::LogCallback::error(debug_output_,
                                 "Invalid compression level '{}'. Asset ID: {}, type ID: {}",
                                 compression_level,
                                 handle.asset_id_, handle.type_id_);
        return Error::kCompressionFailed;
    }

    const auto root = catalog_.root();
    auto archive_path = core::normalizedRelativePath(root / archive, root);
    if(!archive_path) return archive_path.error();

    BakeContext context;
    context.output_bytes = core::Vector<std::byte>(get_allocator());
    context.baked_assets = core::UnorderedSet<AssetId>(get_allocator());
    context.baked_data = core::Vector<AssetCatalog::BakedAsset>(get_allocator());
    context.archive_path = std::move(*archive_path);
    context.compression_level = compression_level;

    if(const auto err = bakeImpl(context, handle))
    {
        return err;
    }

    if(const auto err = core::writeFile(catalog_.root() / context.archive_path, context.output_bytes))
    {
        return err;
    }

    // one lock for the whole list, and no row changes when an id is missing
    if(!catalog_.setBaked(context.archive_path, context.baked_data))
    {
        return Error::kUnknownAsset;
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
            catch(...)
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

void sgf::assets::AssetServer::finishPosted(const core::SharedPtr<PostedJobs>& posted) noexcept
{
    if(!posted) return;

    if(posted->count.fetch_sub(1, std::memory_order_acq_rel) == 1)
    {
        posted->count.notify_all();
    }
}

std::error_code sgf::assets::AssetServer::bakeImpl(BakeContext& context, const AnyHandle& handle)
{
    // already was baked
    if(context.baked_assets.contains(handle.asset_id_)) return {};
    if(handle.asset_id_ == 0 || handle.type_id_ == 0) return {};

    auto* storage = this->storage(core::typeIndexFromStableHash(handle.type_id_));
    if(!storage)
    {
        core::LogCallback::error(debug_output_,
                                 "Cannot find storage. Asset ID: {}, type ID: {}",
                                 handle.asset_id_, handle.type_id_);
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
        core::LogCallback::error(debug_output_,
                                 "Cannot find loader. Asset ID: {}, type ID: {}",
                                 handle.asset_id_, handle.type_id_);
        return Error::kLoaderNotSpecified;
    }

    const auto bake_result = loader->saveBakedErased(slot->erased());
    if(!bake_result)
    {
        return bake_result.error();
    }

    const auto& plain = bake_result->bytes;

    const auto bound = ZSTD_compressBound(plain.size());
    if(ZSTD_isError(bound))
    {
        core::LogCallback::error(debug_output_,
                                 "Cannot compress asset. Error: {}. Asset ID: {}, type ID: {}",
                                 bound,
                                 handle.asset_id_, handle.type_id_);
        return Error::kCompressionFailed;
    }

    core::Vector<std::byte> compressed(bound, get_allocator());
    const auto written = ZSTD_compress(
        compressed.data(), compressed.size(),
        plain.data(), plain.size(),
        context.compression_level);
    if(ZSTD_isError(written))
    {
        core::LogCallback::error(debug_output_,
                                 "Cannot compress asset. Error is: {}, Asset ID: {}, type ID: {}",
                                 written,
                                 handle.asset_id_, handle.type_id_);
        return Error::kCompressionFailed;
    }

    // a zstd frame can be larger than the raw payload. store the raw bytes in that case
    std::span stored = plain;
    if(written < plain.size())
    {
        compressed.resize(written);
        stored = { compressed.data(), compressed.size() };
    }

    // 8 bytes per handle: asset id, then type id
    core::Vector<std::uint32_t> handles_bytes(get_allocator());
    handles_bytes.reserve(bake_result->dependencies.size() * 2);
    for(const auto& dep : bake_result->dependencies)
    {
        handles_bytes.push_back(dep.asset_id_);
        handles_bytes.push_back(dep.type_id_);
    }

    const auto dependencies_bytes = handles_bytes.size() * sizeof(std::uint32_t);

    AssetHeader header;
    header.content_block_size = stored.size();
    header.dependencies_block_size = dependencies_bytes;

    // asset record size
    const auto record_size = sizeof(header) + stored.size() + dependencies_bytes;

    AssetCatalog::BakedAsset baked;
    baked.asset_id = handle.asset_id_;
    baked.offset = context.output_bytes.size();
    baked.size = record_size;

    context.baked_data.push_back(baked);

    context.output_bytes.reserve(context.output_bytes.size() + record_size);

    const auto* header_bytes = reinterpret_cast<const std::byte*>(&header);
    context.output_bytes.insert(context.output_bytes.end(), header_bytes, header_bytes + sizeof(header));
    context.output_bytes.insert(context.output_bytes.end(), stored.begin(), stored.end());
    // if asset has dependencies then inserting the dependencies block
    if(dependencies_bytes != 0)
    {
        const auto* dependency_bytes = reinterpret_cast<const std::byte*>(handles_bytes.data());
        context.output_bytes.insert(context.output_bytes.end(), dependency_bytes, dependency_bytes + dependencies_bytes);
    }

    context.baked_assets.insert(handle.asset_id_);

    // baking all direct dependencies
    for(const auto& dep : bake_result->dependencies)
    {
        if(const auto err = bakeImpl(context, dep))
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
    {
        std::unique_lock lock(output.storages_mutex_);
        output.page_size_ = create_info.page_size;
    }
    output.debug_output_ = create_info.debug_output;
    output.thread_pool_ = create_info.thread_pool;

    return {};
}

sgf::assets::AssetServer& sgf::assets::AssetServer::operator=(AssetServer&& other) noexcept
{
    if(this == &other) return *this;

    // waiting for old tasks
    waitPosted();

    std::unique_lock lock(storages_mutex_, std::defer_lock);
    std::unique_lock other_lock(other.storages_mutex_, std::defer_lock);
    std::lock(lock, other_lock);

    storages_map_ = std::move(other.storages_map_);
    catalog_ = std::move(other.catalog_);
    page_size_ = other.page_size_;
    debug_output_ = other.debug_output_;
    thread_pool_ = other.thread_pool_;
    posted_ = std::move(other.posted_);
    return *this;
}
