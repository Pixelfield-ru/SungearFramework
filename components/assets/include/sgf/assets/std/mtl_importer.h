//
// Created by stuka on 16.11.2026.
//

#pragma once

#include "material.h"
#include "sgf/assets/importer.h"

namespace sgf::assets
{
    struct MtlImporter final : IAssetImporter<Material>
    {
        [[nodiscard]] std::span<const std::string_view> extensions() const override;
        [[nodiscard]] std::error_code importSource(const std::filesystem::path& source, Material& asset) override;
    };
}