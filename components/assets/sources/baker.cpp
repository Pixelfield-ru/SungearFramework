//
// Created by stuka on 16.11.2026.
//

#include "baker.h"

#include "sgf/assets/header.h"

#include <cstring>
#include <filesystem>

#include <zstd.h>

std::error_code sgf::assets::Baker::bake(
    const AnyHandle& handle, const std::filesystem::path& archive, std::int32_t compression_level)
{
    if(compression_level < ZSTD_minCLevel() || compression_level > ZSTD_maxCLevel())
    {
        core::LogCallback::error(server_->debug_output_,
                                 "Invalid compression level '{}'. Asset ID: {}, type ID: {}",
                                 compression_level,
                                 handle.asset_id_, handle.type_id_);
        return Error::kCompressionFailed;
    }

    const auto root = server_->catalog_.root();
    auto archive_path = core::normalizedRelativePath(root / archive, root);
    if(!archive_path) return archive_path.error();

    BakeContext context;
    context.baked_ids = core::UnorderedSet<AssetId>(server_->get_allocator());
    context.output_bytes = core::Vector<std::byte>(server_->get_allocator());
    context.baked_data = core::Vector<AssetCatalog::BakedAsset>(server_->get_allocator());
    context.archive_path = std::move(*archive_path);
    context.compression_level = compression_level;

    if(const auto err = bakeImpl(context, handle))
    {
        return err;
    }

    // every asset already lives in another archive
    if(context.output_bytes.empty()) return {};

    // close the destination archive to write it
    if(auto* mapped = server_->catalog_.findArchive(context.archive_path))
    {
        if(mapped->opened())
        {
            if(const auto err = mapped->close()) return err;
        }
    }

    if(const auto err = core::writeFile(server_->catalog_.root() / context.archive_path, context.output_bytes))
    {
        return err;
    }

    // one lock for the whole list, and no row changes when an id is missing
    if(!server_->catalog_.setBaked(context.archive_path, context.baked_data))
    {
        return Error::kUnknownAsset;
    }

    return server_->catalog_.save();
}

std::error_code sgf::assets::Baker::bakeImpl(BakeContext& context, const AnyHandle& handle)
{
    // already baked
    if(context.baked_ids.contains(handle.asset_id_)) return {};
    context.baked_ids.insert(handle.asset_id_);
    if(handle.asset_id_ == 0 || handle.type_id_ == 0) return {};

    auto* storage = server_->storage(core::typeIndexFromStableHash(handle.type_id_));
    if(!storage)
    {
        core::LogCallback::error(server_->debug_output_,
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
        core::LogCallback::error(server_->debug_output_,
                                 "Cannot find loader. Asset ID: {}, type ID: {}",
                                 handle.asset_id_, handle.type_id_);
        return Error::kLoaderNotSpecified;
    }

    const auto dependencies = loader->dependencies(slot->erased());
    const auto file = server_->catalog_.findFile(handle.asset_id_);
    const auto unchanged = file && file->baked && file->version == slot->version.load();

    std::error_code err;
    // asset was saved in other archive
    if(unchanged && file->baked->archive != context.archive_path)
    {
        err = {};
    }
    // asset is unchanged. copying existing bytes
    else if(unchanged)
    {
        err = bakeOld(context, *file, dependencies);
    }
    // asset was changed. baking new bytes
    else err = bakeFresh(context, *slot, *loader, handle.asset_id_, handle.type_id_, dependencies);

    if(err) return err;
    return bakeDependencies(context, dependencies);
}

std::error_code sgf::assets::Baker::bakeOld(BakeContext& context, const AssetCatalog::File& file,
                                            std::span<const AnyHandle> dependencies)
{
    auto& asset_archive = server_->catalog_.claimArchive(file.baked->archive);
    if(!asset_archive.opened() && std::filesystem::exists(asset_archive.path()))
    {
        if(const auto err = asset_archive.open(std::ios_base::in)) return err;
    }

    const auto archive_bytes = asset_archive.bytes();

    if(file.baked->size < sizeof(AssetHeader) ||
       file.baked->offset > archive_bytes.size() ||
       archive_bytes.size() - file.baked->offset < file.baked->size)
    {
        return std::make_error_code(std::errc::result_out_of_range);
    }

    const auto header_bytes = archive_bytes.subspan(file.baked->offset, sizeof(AssetHeader));
    AssetHeader header;
    std::memcpy(&header, header_bytes.data(), sizeof(AssetHeader));

    if(header.content_block_size > file.baked->size - sizeof(AssetHeader))
    {
        return std::make_error_code(std::errc::result_out_of_range);
    }

    const auto content_bytes = archive_bytes.subspan(file.baked->offset + sizeof(AssetHeader),
                                                     header.content_block_size);

    // the archive file is replaced wholesale, so the old payload is carried over
    appendRecord(context, file.id, file.version, content_bytes, dependencies);
    return {};
}

std::error_code sgf::assets::Baker::bakeFresh(BakeContext& context, AssetSlotControl& slot, IErasedAssetLoader& loader,
                                              AssetId asset_id, std::uint32_t type_id,
                                              std::span<const AnyHandle> dependencies)
{
    const auto baked_bytes = loader.saveBaked(slot.erased());
    if(!baked_bytes)
    {
        return baked_bytes.error();
    }

    const auto bound = ZSTD_compressBound(baked_bytes->size());
    if(ZSTD_isError(bound))
    {
        core::LogCallback::error(server_->debug_output_,
                                 "Cannot compress asset. Error: {}. Asset ID: {}, type ID: {}",
                                 bound,
                                 asset_id, type_id);
        return Error::kCompressionFailed;
    }

    core::Vector<std::byte> compressed(bound, server_->get_allocator());
    const auto written = ZSTD_compress(
        compressed.data(), compressed.size(),
        baked_bytes->data(), baked_bytes->size(),
        context.compression_level);
    if(ZSTD_isError(written))
    {
        core::LogCallback::error(server_->debug_output_,
                                 "Cannot compress asset. Error is: {}, Asset ID: {}, type ID: {}",
                                 written,
                                 asset_id, type_id);
        return Error::kCompressionFailed;
    }

    // a zstd frame can be larger than the raw payload. store the raw bytes in that case
    std::span stored = *baked_bytes;
    if(written < baked_bytes->size())
    {
        compressed.resize(written);
        stored = { compressed.data(), compressed.size() };
    }

    appendRecord(context, asset_id, slot.version.load(), stored, dependencies);

    return {};
}

std::error_code sgf::assets::Baker::bakeDependencies(BakeContext& context, std::span<const AnyHandle> dependencies)
{
    for(const auto& dependency : dependencies)
    {
        if(const auto err = bakeImpl(context, dependency))
        {
            return err;
        }
    }

    return {};
}

void sgf::assets::Baker::appendRecord(BakeContext& context, AssetId asset_id, std::uint32_t version,
                                      std::span<const std::byte> asset_bytes,
                                      std::span<const AnyHandle> dependencies) const
{
    // 8 bytes per handle: asset id, then type id
    core::Vector<std::uint32_t> handles_bytes(server_->get_allocator());
    handles_bytes.reserve(dependencies.size() * 2);
    for(const auto& dep : dependencies)
    {
        handles_bytes.push_back(dep.asset_id_);
        handles_bytes.push_back(dep.type_id_);
    }

    const auto dependencies_bytes = handles_bytes.size() * sizeof(std::uint32_t);

    AssetHeader header;
    header.content_block_size = asset_bytes.size();
    header.dependencies_block_size = dependencies_bytes;
    header.version = version;
    header.asset_id = asset_id;

    // asset record size
    const auto record_size = sizeof(header) + asset_bytes.size() + dependencies_bytes;

    AssetCatalog::BakedAsset baked;
    baked.asset_id = asset_id;
    baked.offset = context.output_bytes.size();
    baked.size = record_size;
    baked.version = version;

    context.baked_data.push_back(baked);

    context.output_bytes.reserve(context.output_bytes.size() + record_size);

    const auto* header_bytes = reinterpret_cast<const std::byte*>(&header);
    context.output_bytes.insert(context.output_bytes.end(), header_bytes, header_bytes + sizeof(header));
    context.output_bytes.insert(context.output_bytes.end(), asset_bytes.begin(), asset_bytes.end());
    // if asset has dependencies then inserting the dependencies block
    if(dependencies_bytes != 0)
    {
        const auto* dependency_bytes = reinterpret_cast<const std::byte*>(handles_bytes.data());
        context.output_bytes.insert(context.output_bytes.end(), dependency_bytes, dependency_bytes + dependencies_bytes);
    }
}
