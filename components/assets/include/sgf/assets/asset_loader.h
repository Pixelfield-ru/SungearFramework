//
// Created by stuka on 25.10.2026.
//

#pragma once

#include <span>
#include <string_view>

namespace sgf::assets
{
    template<typename AssetT>
    struct IAssetLoader
    {
        virtual ~IAssetLoader() = default;
        [[nodiscard]] virtual std::span<const std::string_view> extensions() const noexcept = 0;
    };
}