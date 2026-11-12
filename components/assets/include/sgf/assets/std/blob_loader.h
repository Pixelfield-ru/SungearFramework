//
// Created by stuka on 12.11.2026.
//

#pragma once

#include <expected>

#include "sgf/assets/handle.h"
#include "sgf/assets/loader.h"
#include "sgf/assets/std/blob.h"

namespace sgf::assets
{
    struct BlobLoader final : IAssetLoader<Blob>
    {
        [[nodiscard]] std::error_code loadSource(const std::filesystem::path& source, Blob& asset) override;
        [[nodiscard]] std::error_code loadBaked(std::span<const std::byte> bytes, Blob& asset) override;
        [[nodiscard]] std::expected<BakeResult, std::error_code> saveBaked(const Blob& asset) override;

        [[nodiscard]] std::error_code loadMeta(const std::filesystem::path& source, AssetId id) override;
        [[nodiscard]] std::error_code writeMeta(const std::filesystem::path& source, AssetId id) override;
    };
}
