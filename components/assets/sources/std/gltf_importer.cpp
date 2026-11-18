//
// Created by stuka on 18.11.2026.
//

#include "sgf/assets/std/gltf_importer.h"

#include <array>

#include <fastgltf/parser.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>

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

namespace
{
    glm::mat4 getNodeLocalTransform(const fastgltf::Node& node) noexcept
    {
        // decomposing translation-rotation-scale matrix
        if(std::holds_alternative<fastgltf::TRS>(node.transform))
        {
            const auto& trs = std::get<fastgltf::TRS>(node.transform);
            glm::mat4 m = glm::translate(glm::mat4(1.0f),
                glm::vec3(trs.translation[0], trs.translation[1], trs.translation[2]));
            m = glm::rotate(m, trs.rotation[3],
                glm::vec3(trs.rotation[0], trs.rotation[1], trs.rotation[2]));
            m = glm::scale(m, glm::vec3(trs.scale[0], trs.scale[1], trs.scale[2]));
            return m;
        }

        // ready matrix (column-major)
        const auto& mat = std::get<fastgltf::math::fmat4x4>(node.transform);
        glm::mat4 result;
        std::memcpy(&result, mat.data(), sizeof(float) * 16);
        return result;
    }

    std::error_code processMesh(sgf::assets::Model& model, const fastgltf::Asset& asset, const fastgltf::Mesh& mesh,
                                 const glm::mat4& world_transform)
    {

        return {};
    }

    std::error_code traverseNode(sgf::assets::Model& model, const fastgltf::Asset& asset, size_t node_index,
                                 const glm::mat4& parent_transform)
    {
        const auto& node = asset.nodes[node_index];

        const auto world_transform = parent_transform * getNodeLocalTransform(node);

        // if node has mesh
        if(node.meshIndex.has_value())
        {
            const auto& mesh = asset.meshes[*node.meshIndex];
            if(const auto err = processMesh(model, asset, mesh, world_transform))
            {
                return err;
            }
        }

        // children nodes
        for(const auto child_index : node.children)
        {
            if(const auto err = traverseNode(model, asset, child_index, world_transform))
            {
                return err;
            }
        }

        return {};
    }

    std::error_code processAsset(sgf::assets::Model& model, const fastgltf::Asset& asset)
    {
        for(const auto& scene : asset.scenes)
        {
            for(const auto idx : scene.nodeIndices)
            {
               if(const auto err = traverseNode(model, asset, idx, glm::mat4(1.0f)))
               {
                   return err;
               }
            }
        }

        return {};
    }
}

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

    processAsset(asset, *gltf_asset);

    return {};
}
