//
// Created by stuka on 16.11.2026.
//

#pragma once

#include "image.h"
#include "sgf/assets/importer.h"

namespace sgf::assets
{
    struct PngImporter final : IAssetImporter<Image>
    {
        [[nodiscard]] std::span<const std::string_view> extensions() const override;
        [[nodiscard]] std::error_code importSource(const std::filesystem::path& source, Image& asset) override;
    };
}