//
// Created by stuka on 16.11.2026.
//

#pragma once

#include "sgf/assets/handle.h"
#include "sgf/assets/loader.h"
#include "sgf/assets/std/image.h"

namespace sgf::assets
{
    struct ImageLoader final : IAssetLoader<Image>
    {
        explicit ImageLoader(allocator_type allocator = {}) :
            IAssetLoader(allocator)
        { }

        [[nodiscard]] std::error_code loadBaked(std::span<const std::byte> bytes,
                                                std::span<const AnyHandle> dependencies,
                                                Image& asset) override;
        [[nodiscard]] std::expected<core::Vector<std::byte>, std::error_code> saveBaked(const Image& asset) override;

        [[nodiscard]] std::error_code loadMeta(const std::filesystem::path& source, AssetId id) override;
        [[nodiscard]] std::error_code writeMeta(const std::filesystem::path& source, AssetId id) override;

        [[nodiscard]] core::Vector<AnyHandle> dependencies(const Image& asset) override;
    };
}