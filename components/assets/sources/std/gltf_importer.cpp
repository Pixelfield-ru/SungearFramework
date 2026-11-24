//
// Created by stuka on 18.11.2026.
//

#include "sgf/assets/std/gltf_importer.h"

#include <array>
#include <cstring>
#include <expected>
#include <span>
#include <system_error>
#include <variant>

#include <fastgltf/parser.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/uri.hpp>
#include <fastgltf/glm_element_traits.hpp>

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "sgf/assets/server.h"
#include "sgf/assets/std/image_utils.h"

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
    struct ImportContext final
    {
        sgf::assets::AssetServer* server {};
        sgf::assets::Model* model {};
        const fastgltf::Asset* asset {};
        sgf::core::Vector<bool> visited_nodes;
        sgf::core::Vector<bool> visited_meshes;
        sgf::core::Vector<bool> visited_materials;
        sgf::core::Vector<bool> visited_images;
        sgf::core::Vector<sgf::assets::Handle<sgf::assets::Image>> images;
        std::filesystem::path source;
    };

    struct EncodedImage final
    {
        std::span<const std::byte> bytes;
        fastgltf::MimeType mime = fastgltf::MimeType::None;
    };

    std::expected<std::span<const std::byte>, std::error_code> sourceBytes(const fastgltf::DataSource& source)
    {
        return fastgltf::visit_exhaustive(fastgltf::visitor {
            [](std::monostate) -> std::expected<std::span<const std::byte>, std::error_code> {
                return std::unexpected(std::make_error_code(std::errc::invalid_argument));
            },
            [](const fastgltf::sources::BufferView&) -> std::expected<std::span<const std::byte>, std::error_code> {
                return std::unexpected(std::make_error_code(std::errc::not_supported));
            },
            [](const fastgltf::sources::URI&) -> std::expected<std::span<const std::byte>, std::error_code> {
                return std::unexpected(std::make_error_code(std::errc::not_supported));
            },
            [](const fastgltf::sources::Array& array) -> std::expected<std::span<const std::byte>, std::error_code> {
                return std::span<const std::byte> { array.bytes.data(), array.bytes.size_bytes() };
            },
            [](const fastgltf::sources::Vector& vector) -> std::expected<std::span<const std::byte>, std::error_code> {
                return std::span<const std::byte> { vector.bytes.data(), vector.bytes.size() };
            },
            [](const fastgltf::sources::CustomBuffer&) -> std::expected<std::span<const std::byte>, std::error_code> {
                return std::unexpected(std::make_error_code(std::errc::not_supported));
            },
            [](const fastgltf::sources::ByteView& view) -> std::expected<std::span<const std::byte>, std::error_code> {
                return view.bytes;
            },
            [](const fastgltf::sources::Fallback&) -> std::expected<std::span<const std::byte>, std::error_code> {
                return std::unexpected(std::make_error_code(std::errc::invalid_argument));
            },
        }, source);
    }

    std::expected<EncodedImage, std::error_code> encodedImage(const fastgltf::Asset& asset, const fastgltf::Image& image)
    {
        return fastgltf::visit_exhaustive(fastgltf::visitor {
            [](std::monostate) -> std::expected<EncodedImage, std::error_code> {
                return std::unexpected(std::make_error_code(std::errc::invalid_argument));
            },
            [&](const fastgltf::sources::BufferView& view) -> std::expected<EncodedImage, std::error_code> {
                if(view.bufferViewIndex >= asset.bufferViews.size())
                {
                    return std::unexpected(std::make_error_code(std::errc::result_out_of_range));
                }

                const auto& buffer_view = asset.bufferViews[view.bufferViewIndex];
                if(buffer_view.bufferIndex >= asset.buffers.size())
                {
                    return std::unexpected(std::make_error_code(std::errc::result_out_of_range));
                }

                auto bytes = sourceBytes(asset.buffers[buffer_view.bufferIndex].data);
                if(!bytes) return std::unexpected(bytes.error());
                if(buffer_view.byteOffset > bytes->size() ||
                   buffer_view.byteLength > bytes->size() - buffer_view.byteOffset)
                {
                    return std::unexpected(std::make_error_code(std::errc::result_out_of_range));
                }

                return EncodedImage {
                    bytes->subspan(buffer_view.byteOffset, buffer_view.byteLength),
                    view.mimeType
                };
            },
            [](const fastgltf::sources::URI&) -> std::expected<EncodedImage, std::error_code> {
                return std::unexpected(std::make_error_code(std::errc::not_supported));
            },
            [](const fastgltf::sources::Array& array) -> std::expected<EncodedImage, std::error_code> {
                return EncodedImage {
                    { array.bytes.data(), array.bytes.size_bytes() },
                    array.mimeType
                };
            },
            [](const fastgltf::sources::Vector& vector) -> std::expected<EncodedImage, std::error_code> {
                return EncodedImage {
                    { vector.bytes.data(), vector.bytes.size() },
                    vector.mimeType
                };
            },
            [](const fastgltf::sources::CustomBuffer&) -> std::expected<EncodedImage, std::error_code> {
                return std::unexpected(std::make_error_code(std::errc::not_supported));
            },
            [](const fastgltf::sources::ByteView& view) -> std::expected<EncodedImage, std::error_code> {
                return EncodedImage { view.bytes, view.mimeType };
            },
            [](const fastgltf::sources::Fallback&) -> std::expected<EncodedImage, std::error_code> {
                return std::unexpected(std::make_error_code(std::errc::invalid_argument));
            },
        }, image.data);
    }

    std::error_code supportedImageMime(fastgltf::MimeType mime)
    {
        switch(mime)
        {
            case fastgltf::MimeType::None:
            case fastgltf::MimeType::JPEG:
            case fastgltf::MimeType::PNG:
            case fastgltf::MimeType::OctetStream:
                return {};
            default:
                return std::make_error_code(std::errc::not_supported);
        }
    }

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

    sgf::assets::Material::AlphaMode mapAlphaMode(fastgltf::AlphaMode mode) noexcept
    {
        switch(mode)
        {
            case fastgltf::AlphaMode::Opaque: return sgf::assets::Material::AlphaMode::kOpaque;
            case fastgltf::AlphaMode::Mask: return sgf::assets::Material::AlphaMode::kMask;
            case fastgltf::AlphaMode::Blend: return sgf::assets::Material::AlphaMode::kBlend;
        }

        return sgf::assets::Material::AlphaMode::kOpaque;
    }

    void assignMaterial(sgf::assets::Material& material, const fastgltf::Material& src)
    {
        material.base_color = glm::vec4(src.pbrData.baseColorFactor[0], src.pbrData.baseColorFactor[1],
                                        src.pbrData.baseColorFactor[2], src.pbrData.baseColorFactor[3]);
        material.metallic = src.pbrData.metallicFactor;
        material.roughness = src.pbrData.roughnessFactor;
        material.emissive = glm::vec3(src.emissiveFactor[0], src.emissiveFactor[1], src.emissiveFactor[2]);
        material.alpha_cutoff = src.alphaCutoff;
        material.double_sided = src.doubleSided;
        material.alpha_mode = mapAlphaMode(src.alphaMode);
    }

    std::error_code takeImageHandle(ImportContext& context,
                                    std::size_t image_index,
                                    std::expected<sgf::assets::Handle<sgf::assets::Image>, std::error_code> handle)
    {
        if(!handle) return handle.error();

        auto& image_handle = *handle;
        if(!image_handle)
        {
            if(image_handle.state() == sgf::assets::AssetState::kLoading) return sgf::assets::Error::kLoading;
            if(const auto err = image_handle.error()) return err;
            return sgf::assets::Error::kNotLoaded;
        }

        context.visited_images[image_index] = true;
        context.images[image_index] = std::move(image_handle);
        return {};
    }

    std::error_code ensureImageFile(ImportContext& context, std::size_t image_index, const fastgltf::sources::URI& source)
    {
        if(source.uri.valueless_after_move() || !source.uri->valid())
        {
            return std::make_error_code(std::errc::invalid_argument);
        }
        if(!source.uri->isLocalPath() || source.fileByteOffset != 0)
        {
            return std::make_error_code(std::errc::not_supported);
        }
        if(const auto err = supportedImageMime(source.mimeType)) return err;

        const auto uri_path = source.uri->fspath();
        const auto file_path = uri_path.is_absolute() ? uri_path : context.source.parent_path() / uri_path;

        // the read runs on the pool. this import keeps the handle and does not wait
        auto handle = context.server->load<sgf::assets::Image>(file_path);
        if(!handle) return handle.error();

        context.visited_images[image_index] = true;
        context.images[image_index] = std::move(*handle);
        return {};
    }

    std::error_code ensureImage(ImportContext& context, std::size_t image_index)
    {
        if(image_index >= context.images.size())
        {
            return std::make_error_code(std::errc::result_out_of_range);
        }
        if(context.visited_images[image_index]) return {};

        const auto& gltf_image = context.asset->images[image_index];
        if(const auto* uri = std::get_if<fastgltf::sources::URI>(&gltf_image.data))
        {
            return ensureImageFile(context, image_index, *uri);
        }

        const auto encoded = encodedImage(*context.asset, gltf_image);
        if(!encoded) return encoded.error();
        if(const auto err = supportedImageMime(encoded->mime)) return err;

        sgf::assets::Image decoded(context.model->get_allocator());
        if(const auto err = sgf::assets::utils::decodeImage(encoded->bytes, decoded)) return err;

        const auto image_path = std::filesystem::path(context.source).concat(".images") / std::to_string(image_index);
        auto handle = context.server->ensure<sgf::assets::Image>(image_path, [](sgf::assets::Image&) {
            return std::error_code {};
        });
        if(!handle) return handle.error();

        auto& image_handle = *handle;
        if(!image_handle)
        {
            if(image_handle.state() == sgf::assets::AssetState::kLoading) return sgf::assets::Error::kLoading;
            if(const auto err = image_handle.error()) return err;
            return sgf::assets::Error::kNotLoaded;
        }

        image_handle.edit([&](sgf::assets::Image& image) {
            image = std::move(decoded);
        });

        return takeImageHandle(context, image_index, std::move(handle));
    }

    template<typename OptionalTextureT>
    const fastgltf::TextureInfo* textureInfo(const OptionalTextureT& texture)
    {
        if(!texture) return nullptr;
        return &*texture;
    }

    std::error_code loadTextureImage(ImportContext& context,
                                     const fastgltf::TextureInfo* info,
                                     sgf::assets::Handle<sgf::assets::Image>& destination)
    {
        destination = nullptr;
        if(!info) return {};

        if(info->texCoordIndex != 0)
        {
            return std::make_error_code(std::errc::invalid_argument);
        }
        if(info->transform && info->transform->texCoordIndex && *info->transform->texCoordIndex != 0)
        {
            return std::make_error_code(std::errc::invalid_argument);
        }
        if(info->textureIndex >= context.asset->textures.size())
        {
            return std::make_error_code(std::errc::result_out_of_range);
        }

        const auto& texture = context.asset->textures[info->textureIndex];
        if(!texture.imageIndex) return std::make_error_code(std::errc::not_supported);

        const auto image_index = *texture.imageIndex;
        if(const auto err = ensureImage(context, image_index)) return err;

        destination = context.images[image_index];
        return {};
    }

    sgf::assets::Model::PrimitiveType mapPrimitiveType(fastgltf::PrimitiveType type) noexcept
    {
        switch(type)
        {
            case fastgltf::PrimitiveType::Points: return sgf::assets::Model::PrimitiveType::kPoints;
            case fastgltf::PrimitiveType::Lines: return sgf::assets::Model::PrimitiveType::kLines;
            case fastgltf::PrimitiveType::LineLoop: return sgf::assets::Model::PrimitiveType::kLineLoop;
            case fastgltf::PrimitiveType::LineStrip: return sgf::assets::Model::PrimitiveType::kLineStrip;
            case fastgltf::PrimitiveType::Triangles: return sgf::assets::Model::PrimitiveType::kTriangles;
            case fastgltf::PrimitiveType::TriangleStrip: return sgf::assets::Model::PrimitiveType::kTriangleStrip;
            case fastgltf::PrimitiveType::TriangleFan: return sgf::assets::Model::PrimitiveType::kTriangleFan;
        }

        return sgf::assets::Model::PrimitiveType::kTriangles;
    }

    std::error_code processMesh(ImportContext& context, std::size_t mesh_index)
    {
        auto& gltf_mesh = context.asset->meshes[mesh_index];

        auto& mesh = context.model->meshes[mesh_index];
        mesh.name = gltf_mesh.name;

        for(const auto& gltf_primitive : gltf_mesh.primitives)
        {
            auto& primitive = mesh.primitives.emplace_back();
            primitive.type = mapPrimitiveType(gltf_primitive.type);

            if(gltf_primitive.materialIndex)
            {
                const auto material_index = *gltf_primitive.materialIndex;
                if(material_index >= context.asset->materials.size())
                {
                    return std::make_error_code(std::errc::result_out_of_range);
                }

                primitive.material = static_cast<std::uint32_t>(material_index);
                if(!context.visited_materials[material_index])
                {
                    context.visited_materials[material_index] = true;

                    const auto material_path = std::filesystem::path(context.source).concat(".materials") / std::to_string(material_index);
                    auto handle = context.server->ensure<sgf::assets::Material>(material_path, [](sgf::assets::Material&) {
                        return std::error_code {};
                    });

                    if(!handle) return handle.error();

                    auto& material_handle = *handle;
                    if(!material_handle)
                    {
                        if(material_handle.state() == sgf::assets::AssetState::kLoading) return sgf::assets::Error::kLoading;
                        if(const auto err = material_handle.error()) return err;
                        return sgf::assets::Error::kNotLoaded;
                    }

                    const auto& src = context.asset->materials[material_index];
                    sgf::assets::Handle<sgf::assets::Image> base_color_image;
                    sgf::assets::Handle<sgf::assets::Image> metallic_roughness_image;
                    sgf::assets::Handle<sgf::assets::Image> normal_image;
                    sgf::assets::Handle<sgf::assets::Image> occlusion_image;
                    sgf::assets::Handle<sgf::assets::Image> emissive_image;

                    if(const auto err = loadTextureImage(context, textureInfo(src.pbrData.baseColorTexture), base_color_image))
                    {
                        return err;
                    }
                    if(const auto err = loadTextureImage(context, textureInfo(src.pbrData.metallicRoughnessTexture), metallic_roughness_image))
                    {
                        return err;
                    }
                    if(const auto err = loadTextureImage(context, textureInfo(src.normalTexture), normal_image))
                    {
                        return err;
                    }
                    if(const auto err = loadTextureImage(context, textureInfo(src.occlusionTexture), occlusion_image))
                    {
                        return err;
                    }
                    if(const auto err = loadTextureImage(context, textureInfo(src.emissiveTexture), emissive_image))
                    {
                        return err;
                    }

                    material_handle.edit([&](sgf::assets::Material& material) {
                        assignMaterial(material, src);
                        material.base_color_image = std::move(base_color_image);
                        material.metallic_roughness_image = std::move(metallic_roughness_image);
                        material.normal_image = std::move(normal_image);
                        material.occlusion_image = std::move(occlusion_image);
                        material.emissive_image = std::move(emissive_image);
                    });
                    context.model->materials[material_index] = std::move(material_handle);
                }
            }

            const auto copy_attribute = [&]<typename FieldT>(std::string_view attribute_name,
                                                             FieldT sgf::assets::Model::Vertex::* member,
                                                             bool required)
            {
                const auto values = gltf_primitive.findAttribute(attribute_name);
                if(values == gltf_primitive.attributes.end())
                {
                    return required ? std::make_error_code(std::errc::invalid_argument) : std::error_code {};
                }
                if(values->accessorIndex >= context.asset->accessors.size())
                {
                    return std::make_error_code(std::errc::result_out_of_range);
                }

                const auto& accessor = context.asset->accessors[values->accessorIndex];
                if(required)
                {
                    primitive.vertices.resize(accessor.count);
                }
                else if(accessor.count != primitive.vertices.size())
                {
                    return std::make_error_code(std::errc::invalid_argument);
                }

                if(accessor.count == 0) return std::error_code {};

                fastgltf::copyFromAccessor<FieldT, sizeof(sgf::assets::Model::Vertex)>(
                    *context.asset, accessor, &(primitive.vertices[0].*member));

                return std::error_code {};
            };

            // getting all attributes

            if(const auto err = copy_attribute.operator()<glm::vec3>("POSITION", &sgf::assets::Model::Vertex::position, true))
            {
                return err;
            }

            if(const auto err = copy_attribute.operator()<glm::vec3>("NORMAL", &sgf::assets::Model::Vertex::normal, false))
            {
                return err;
            }

            if(const auto err = copy_attribute.operator()<glm::vec4>("TANGENT", &sgf::assets::Model::Vertex::tangent, false))
            {
                return err;
            }

            if(const auto err = copy_attribute.operator()<glm::vec2>("TEXCOORD_0", &sgf::assets::Model::Vertex::texcoord, false))
            {
                return err;
            }

            // get indices from primitive
            if(gltf_primitive.indicesAccessor)
            {
                const auto accessor_index = *gltf_primitive.indicesAccessor;
                if(accessor_index >= context.asset->accessors.size())
                {
                    return std::make_error_code(std::errc::result_out_of_range);
                }

                const auto& accessor = context.asset->accessors[accessor_index];
                if(accessor.type != fastgltf::AccessorType::Scalar)
                {
                    return std::make_error_code(std::errc::invalid_argument);
                }

                primitive.indices.resize(accessor.count);
                if(accessor.count > 0)
                {
                    fastgltf::copyFromAccessor<std::uint32_t>(*context.asset, accessor, primitive.indices.data());
                }
            }
        }

        return {};
    }

    std::error_code traverseNode(ImportContext& context, std::size_t node_index)
    {
        auto& model = *context.model;
        auto& asset = *context.asset;
        auto& visited_nodes = context.visited_nodes;
        auto& visited_meshes = context.visited_meshes;

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
                if(const auto err = processMesh(context, mesh_index))
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
            if(const auto err = traverseNode(context, child_index))
            {
                return err;
            }
        }

        return {};
    }

    std::error_code processAsset(ImportContext& context)
    {
        auto& model = *context.model;
        auto& asset = *context.asset;

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
        model.materials.resize(asset.materials.size());
        model.scenes.reserve(asset.scenes.size());

        context.visited_nodes = sgf::core::Vector<bool>(asset.nodes.size(), false, alloc);
        context.visited_meshes = sgf::core::Vector<bool>(asset.meshes.size(), false, alloc);
        context.visited_materials = sgf::core::Vector<bool>(asset.materials.size(), false, alloc);
        context.visited_images = sgf::core::Vector<bool>(asset.images.size(), false, alloc);
        context.images = sgf::core::Vector<sgf::assets::Handle<sgf::assets::Image>>(asset.images.size(), alloc);

        for(const auto& gltf_scene : asset.scenes)
        {
            auto& model_scene = model.scenes.emplace_back();
            model_scene.roots.reserve(gltf_scene.nodeIndices.size());
            model_scene.name = gltf_scene.name;

            for(const auto root_idx : gltf_scene.nodeIndices)
            {
                model_scene.roots.push_back(static_cast<std::uint32_t>(root_idx));
                if(const auto err = traverseNode(context, root_idx))
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

std::error_code sgf::assets::GltfImporter::importSource(const std::filesystem::path& source, Model& asset, AssetServer& server)
{
    fastgltf::Parser parser;
    auto data = fastgltf::GltfDataBuffer::FromPath(source);
    if(!data)
    {
        return data.error();
    }

    constexpr auto options = fastgltf::Options::LoadExternalBuffers
                           | fastgltf::Options::DecomposeNodeMatrices;

    auto gltf_asset = parser.loadGltf(*data, source.parent_path(), options);
    if(!gltf_asset)
    {
        return gltf_asset.error();
    }

    ImportContext context;
    context.server = &server;
    context.model = &asset;
    context.asset = &*gltf_asset;
    context.source = source;

    return processAsset(context);
}
