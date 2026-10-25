//
// Created by stuka on 25.10.2026.
//

#pragma once

#include <atomic>

namespace sgf::assets
{
    template<typename AssetT>
    struct AssetSlot final
    {
        AssetT value {};

    private:
        std::atomic<std::uint32_t> strong_refs_ {};
        std::atomic<std::uint32_t> weak_refs_ {};
    };
}