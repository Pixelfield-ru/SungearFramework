//
// Created by stuka on 12.11.2026.
//

#include "sgf/assets/std/blob_loader.h"

#include "sgf/core/utility/file.h"

std::error_code sgf::assets::BlobLoader::loadSource(const std::filesystem::path& source, Blob& asset)
{
    auto bytes = core::readFile(source, asset.bytes.get_allocator());
    if(!bytes) return bytes.error();

    asset.bytes = std::move(*bytes);
    return {};
}

std::error_code sgf::assets::BlobLoader::loadBaked(std::span<const std::byte> bytes, Blob& asset)
{
    asset.bytes.assign(bytes.begin(), bytes.end());
    return {};
}

std::expected<sgf::assets::BakeResult, std::error_code> sgf::assets::BlobLoader::saveBaked(const Blob& asset)
{
    BakeResult result;
    result.bytes = core::Vector<std::byte>(asset.bytes, asset.bytes.get_allocator());
    return result;
}

std::error_code sgf::assets::BlobLoader::loadMeta(const std::filesystem::path&, AssetId)
{
    return {};
}

std::error_code sgf::assets::BlobLoader::writeMeta(const std::filesystem::path&, AssetId)
{
    return {};
}
