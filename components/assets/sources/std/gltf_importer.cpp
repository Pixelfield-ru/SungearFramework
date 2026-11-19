//
// Created by stuka on 18.11.2026.
//

#include "sgf/assets/std/gltf_importer.h"

#include <array>
#include <cstring>
#include <system_error>

#include <fastgltf/parser.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

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
        if(std::holds_alternative<fastgltf::TRS>(node.transform))
        {
            const auto& trs = std::get<fastgltf::TRS>(node.transform);
            const glm::mat4 translation = glm::translate(glm::mat4(1.0f),
                glm::vec3(trs.translation[0], trs.translation[1], trs.translation[2]));
            const glm::quat rotation(trs.rotation[3], trs.rotation[0], trs.rotation[1], trs.rotation[2]);
            const glm::mat4 scale = glm::scale(glm::mat4(1.0f),
                glm::vec3(trs.scale[0], trs.scale[1], trs.scale[2]));
            return translation * glm::mat4_cast(rotation) * scale;
        }

        const auto& mat = std::get<fastgltf::math::fmat4x4>(node.transform);
        glm::mat4 result;
        std::memcpy(&result, mat.data(), sizeof(float) * 16);
        return result;
    }

    std::error_code processMesh(sgf::assets::Model& model, const fastgltf::Asset& asset, std::size_t mesh_index)
    {
        std::ignore = model;
        std::ignore = asset;
        std::ignore = mesh_index;

        return {};
    }

    std::error_code traverseNode(sgf::assets::Model& model, const fastgltf::Asset& asset, std::size_t node_index,
                                 sgf::core::Vector<bool>& visited_nodes,
                                 sgf::core::Vector<bool>& visited_meshes)
    {
        if(node_index >= model.nodes.size()) return std::make_error_code(std::errc::result_out_of_range);
        if(visited_nodes[node_index]) return {};
        visited_nodes[node_index] = true;

        const auto& gltf_node = asset.nodes[node_index];
        auto& model_node = model.nodes[node_index];

        model_node.name = gltf_node.name;
        model_node.local = getNodeLocalTransform(gltf_node);

        if(gltf_node.meshIndex.has_value())
        {
            const auto mesh_index = *gltf_node.meshIndex;
            if(mesh_index >= model.meshes.size()) return std::make_error_code(std::errc::result_out_of_range);
            model_node.mesh = static_cast<std::uint32_t>(mesh_index);

            if(!visited_meshes[mesh_index])
            {
                visited_meshes[mesh_index] = true;
                if(const auto err = processMesh(model, asset, mesh_index))
                {
                    return err;
                }
            }
        }

        auto& children = model.nodes[node_index].children;
        children.reserve(gltf_node.children.size());
        for(const auto child_index : gltf_node.children)
        {
            children.push_back(static_cast<std::uint32_t>(child_index));
            if(const auto err = traverseNode(model, asset, child_index, visited_nodes, visited_meshes))
            {
                return err;
            }
        }

        return {};
    }

    std::error_code processAsset(sgf::assets::Model& model, const fastgltf::Asset& asset)
    {
        model.meshes.clear();
        model.nodes.clear();
        model.materials.clear();
        model.scenes.clear();
        model.default_scene = 0;

        if(asset.defaultScene && *asset.defaultScene >= asset.scenes.size())
        {
            return std::make_error_code(std::errc::result_out_of_range);
        }

        const auto alloc = model.get_allocator();
        model.meshes.resize(asset.meshes.size());
        model.nodes.resize(asset.nodes.size());
        model.scenes.reserve(asset.scenes.size());

        sgf::core::Vector<bool> visited_nodes(asset.nodes.size(), 0, alloc);
        sgf::core::Vector<bool> visited_meshes(asset.meshes.size(), 0, alloc);

        for(const auto& gltf_scene : asset.scenes)
        {
            auto& model_scene = model.scenes.emplace_back();
            model_scene.roots.reserve(gltf_scene.nodeIndices.size());
            model_scene.name = gltf_scene.name;

            for(const auto root_idx : gltf_scene.nodeIndices)
            {
                model_scene.roots.push_back(static_cast<std::uint32_t>(root_idx));
                if(const auto err = traverseNode(model, asset, root_idx, visited_nodes, visited_meshes))
                {
                    return err;
                }
            }
        }

        model.default_scene = asset.defaultScene
            ? static_cast<std::uint32_t>(*asset.defaultScene)
            : 0;

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

    return processAsset(asset, *gltf_asset);
}
