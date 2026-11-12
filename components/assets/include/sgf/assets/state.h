//
// Created by stuka on 25.10.2026.
//

#pragma once

#include <cstdint>

namespace sgf::assets
{
    enum class AssetState : std::uint8_t
    {
        kUnloaded,
        kLoaded,
        kLoading,
        kFailed
    };
}