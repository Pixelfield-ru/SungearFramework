//
// Created by stuka on 25.10.2026.
//

#pragma once

#include <expected>
#include <filesystem>
#include <span>
#include <string_view>
#include <system_error>

namespace sgf::assets
{
    template<typename AssetT>
    struct IAssetLoader
    {
        virtual ~IAssetLoader() = default;
        [[nodiscard]] virtual std::span<const std::string_view> extensions() const noexcept = 0;
        [[nodiscard]] virtual std::expected<AssetT, std::error_code> loadSource(const std::filesystem::path& source) = 0;
        [[nodiscard]] virtual std::expected<AssetT, std::error_code> loadBaked(std::span<const std::byte> bytes) = 0;
    };
}
