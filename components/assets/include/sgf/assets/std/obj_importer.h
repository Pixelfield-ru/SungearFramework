//
// Created by stuka on 18.11.2026.
//

#pragma once

#include "model.h"
#include "sgf/assets/importer.h"

namespace sgf::assets
{
    struct ObjImporter final : IAssetImporter<Model>
    {
        [[nodiscard]] std::span<const std::string_view> extensions() const override;
        [[nodiscard]] std::error_code importSource(const std::filesystem::path& source, Model& asset, AssetServer& server) override;
    };
}