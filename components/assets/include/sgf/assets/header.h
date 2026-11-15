//
// Created by stuka on 15.11.2026.
//

#pragma once

#include <cstdint>

namespace sgf::assets
{
#pragma pack(push, 1)
    struct AssetHeader final
    {
        std::uint64_t content_block_size {};
        std::uint64_t dependencies_block_size {};
        std::uint32_t version {};
        AssetId asset_id {};
    };
#pragma pack(pop)
}
