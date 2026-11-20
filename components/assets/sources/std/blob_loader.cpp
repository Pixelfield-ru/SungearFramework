//
// Created by stuka on 12.11.2026.
//

#include "sgf/assets/std/blob_loader.h"

#include "sgf/core/utility/file.h"

std::error_code sgf::assets::BlobLoader::loadSource(const std::filesystem::path& source, Blob& asset, AssetServer&)
{
    auto bytes = core::readFile(source, asset.bytes.get_allocator());
    if(!bytes) return bytes.error();

    asset.bytes = std::move(*bytes);
    return {};
}

std::error_code sgf::assets::BlobLoader::loadBaked(
    std::span<const std::byte> bytes,
    std::span<const AnyHandle>,
    Blob& asset)
{
    asset.bytes.assign(bytes.begin(), bytes.end());
    return {};
}

std::expected<sgf::core::Vector<std::byte>, std::error_code> sgf::assets::BlobLoader::saveBaked(const Blob& asset)
{
    return core::Vector<std::byte>(asset.bytes, asset.bytes.get_allocator());
}

std::error_code sgf::assets::BlobLoader::loadMeta(const std::filesystem::path&, AssetId)
{
    return {};
}

std::error_code sgf::assets::BlobLoader::writeMeta(const std::filesystem::path&, AssetId)
{
    return {};
}

sgf::core::Vector<sgf::assets::AnyHandle> sgf::assets::BlobLoader::dependencies(const Blob& asset)
{
    return {};
}
