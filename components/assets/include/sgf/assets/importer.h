//
// Created by stuka on 16.11.2026.
//

#pragma once

#include <filesystem>
#include <span>
#include <string_view>
#include <system_error>

namespace sgf::assets
{
    struct AssetServer;

    template<typename AssetT>
    struct IAssetImporter
    {
        virtual ~IAssetImporter() = default;

        [[nodiscard]] virtual std::span<const std::string_view> extensions() const = 0;
        [[nodiscard]] virtual std::error_code importSource(
            const std::filesystem::path& source, AssetT& asset, AssetServer& server) = 0;
    };
}
