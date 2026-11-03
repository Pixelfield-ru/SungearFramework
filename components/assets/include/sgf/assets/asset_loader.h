//
// Created by stuka on 25.10.2026.
//

#pragma once

#include <expected>
#include <filesystem>
#include <span>
#include <system_error>

namespace sgf::assets
{
    template<typename AssetT>
    struct IAssetLoader
    {
        virtual ~IAssetLoader() = default;
        [[nodiscard]] virtual std::error_code loadSource(const std::filesystem::path& source, AssetT& asset) = 0;
        [[nodiscard]] virtual std::error_code loadBaked(std::span<const std::byte> bytes, AssetT& asset) = 0;
        [[nodiscard]] virtual std::error_code writeMeta(const std::filesystem::path& source, AssetId id) = 0;
    };
}
