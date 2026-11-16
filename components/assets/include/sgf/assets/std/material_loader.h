//
// Created by stuka on 16.11.2026.
//

#pragma once

#include "sgf/assets/handle.h"
#include "sgf/assets/loader.h"
#include "sgf/assets/std/material.h"

namespace sgf::assets
{
    struct MaterialLoader final : IAssetLoader<Material>
    {
        explicit MaterialLoader(allocator_type allocator = {}) :
            IAssetLoader(allocator)
        { }

        [[nodiscard]] std::error_code loadBaked(std::span<const std::byte> bytes,
                                                std::span<const AnyHandle> dependencies,
                                                Material& asset) override;
        [[nodiscard]] std::expected<core::Vector<std::byte>, std::error_code> saveBaked(const Material& asset) override;

        [[nodiscard]] std::error_code loadMeta(const std::filesystem::path& source, AssetId id) override;
        [[nodiscard]] std::error_code writeMeta(const std::filesystem::path& source, AssetId id) override;

        [[nodiscard]] core::Vector<AnyHandle> dependencies(const Material& asset) override;
    };
}