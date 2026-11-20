//
// Created by stuka on 12.11.2026.
//

#pragma once

#include "sgf/assets/handle.h"
#include "sgf/assets/loader.h"
#include "sgf/assets/std/blob.h"

namespace sgf::assets
{
    struct BlobLoader final : IAssetLoader<Blob>
    {
        explicit BlobLoader(allocator_type allocator = {}) :
            IAssetLoader(allocator)
        { }

        [[nodiscard]] std::error_code loadSource(const std::filesystem::path& source, Blob& asset, AssetServer& server) override;
        [[nodiscard]] std::error_code loadBaked(std::span<const std::byte> bytes,
                                                std::span<const AnyHandle> dependencies,
                                                Blob& asset) override;
        [[nodiscard]] std::expected<core::Vector<std::byte>, std::error_code> saveBaked(const Blob& asset) override;

        [[nodiscard]] std::error_code loadMeta(const std::filesystem::path& source, AssetId id) override;
        [[nodiscard]] std::error_code writeMeta(const std::filesystem::path& source, AssetId id) override;

        [[nodiscard]] core::Vector<AnyHandle> dependencies(const Blob& asset) override;

        std::error_code addImporter(core::UniquePtr<IAssetImporter<Blob>>) override
        {
            return std::make_error_code(std::errc::not_supported);
        }
    };
}
