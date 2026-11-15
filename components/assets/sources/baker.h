//
// Created by stuka on 16.11.2026.
//

#pragma once

#include "sgf/assets/server.h"

#include <span>

namespace sgf::assets
{
    class Baker final
    {
    public:
        explicit Baker(AssetServer& server) noexcept :
            server_(&server)
        {}

        std::error_code bake(const AnyHandle& handle, const std::filesystem::path& archive,
                             std::int32_t compression_level);

    private:
        struct BakeContext final
        {
            core::UnorderedSet<AssetId> baked_ids;

            core::Vector<std::byte> output_bytes;
            core::Vector<AssetCatalog::BakedAsset> baked_data;
            std::filesystem::path archive_path;
            std::int32_t compression_level {};
        };

        std::error_code bakeImpl(BakeContext& context, const AnyHandle& handle);
        /// Bake asset that was already saved and was not changed.
        std::error_code bakeOld(BakeContext& context, const AssetCatalog::File& file,
                                std::span<const AnyHandle> dependencies);
        /// Bake asset that was changed.
        std::error_code bakeFresh(BakeContext& context, AssetSlotControl& slot, IErasedAssetLoader& loader,
                                  AssetId asset_id, std::uint32_t type_id,
                                  std::span<const AnyHandle> dependencies);
        std::error_code bakeDependencies(BakeContext& context, std::span<const AnyHandle> dependencies);
        void appendRecord(BakeContext& context, AssetId asset_id, std::uint32_t version,
                          std::span<const std::byte> asset_bytes,
                          std::span<const AnyHandle> dependencies) const;

        AssetServer* server_;
    };
}
