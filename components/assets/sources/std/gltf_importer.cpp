//
// Created by stuka on 18.11.2026.
//

#include "sgf/assets/std/gltf_importer.h"

#include <array>

#include <fastgltf/parser.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>

namespace fastgltf
{
    namespace
    {
        struct FastGltfErrorCategory final : public std::error_category
        {
            [[nodiscard]] const char* name() const noexcept override { return "GltfImporter"; }

            [[nodiscard]] std::string message(int ev) const override
            {
                return { getErrorName(static_cast<Error>(ev)).data() };
            }
        };
    }

    inline const std::error_category& error_category()
    {
        static const FastGltfErrorCategory cat;
        return cat;
    }

    inline std::error_code make_error_code(Error e) noexcept
    {
        return { static_cast<int>(e), error_category() };
    }
}

namespace std
{
    template<>
    struct is_error_code_enum<fastgltf::Error> : true_type {};
}

// impl

std::span<const std::string_view> sgf::assets::GltfImporter::extensions() const
{
    static constexpr std::array<std::string_view, 2> ext { ".gltf", ".glb" };
    return ext;
}

std::error_code sgf::assets::GltfImporter::importSource(const std::filesystem::path& source, Model& asset)
{
    fastgltf::Parser parser;
    auto data = fastgltf::GltfDataBuffer::FromPath(source);
    if(!data)
    {
        return data.error();
    }

    constexpr auto options = fastgltf::Options::LoadExternalBuffers
                           | fastgltf::Options::LoadExternalImages
                           | fastgltf::Options::DecomposeNodeMatrices;

    auto gltf_asset = parser.loadGltf(*data, source.parent_path(), options);
    if(!gltf_asset)
    {
        return gltf_asset.error();
    }

    // todo: process asset

    return {};
}
