//
// Created by stuka on 25.10.2026.
//

#pragma once

#include <filesystem>
#include <span>
#include <system_error>

#include "sgf/core/memory.h"
#include "asset_slot.h"

namespace sgf::assets
{
    struct BakeResult final
    {
        /// Bytes of asset.
        core::Vector<std::byte> bytes;
        /// Direct dependencies of asset.
        core::Vector<AnyHandle> dependencies;
    };

    struct IErasedAssetLoader
    {
        virtual ~IErasedAssetLoader() = default;
        [[nodiscard]] virtual std::error_code loadSourceErased(const std::filesystem::path& source, std::byte* asset) = 0;
        [[nodiscard]] virtual std::error_code loadBakedErased(std::span<const std::byte> source, std::byte* asset) = 0;
        [[nodiscard]] virtual std::expected<BakeResult, std::error_code> saveBakedErased(std::byte const* asset) = 0;

        [[nodiscard]] virtual std::error_code loadMeta(const std::filesystem::path& source, AssetId id) = 0;
        [[nodiscard]] virtual std::error_code writeMeta(const std::filesystem::path& source, AssetId id) = 0;
    };

    template<typename AssetT>
    struct IAssetLoader : IErasedAssetLoader
    {
        ~IAssetLoader() override = default;
        [[nodiscard]] virtual std::error_code loadSource(const std::filesystem::path& source, AssetT& asset) = 0;
        [[nodiscard]] std::error_code loadSourceErased(const std::filesystem::path& source, std::byte* asset) noexcept final
        {
            return loadSource(source, *reinterpret_cast<AssetT*>(asset));
        }
        [[nodiscard]] virtual std::error_code loadBaked(std::span<const std::byte> bytes, AssetT& asset) = 0;
        [[nodiscard]] std::error_code loadBakedErased(std::span<const std::byte> source, std::byte* asset) noexcept final
        {
            return loadBaked(source, *reinterpret_cast<AssetT*>(asset));
        }
        [[nodiscard]] virtual std::expected<BakeResult, std::error_code> saveBaked(const AssetT& asset) = 0;
        [[nodiscard]] std::expected<BakeResult, std::error_code> saveBakedErased(std::byte const* asset) noexcept final
        {
            return saveBaked(*reinterpret_cast<AssetT*>(asset));
        }
    };
}
