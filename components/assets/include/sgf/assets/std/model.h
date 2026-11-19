//
// Created by stuka on 18.11.2026.
//

#pragma once

#include <vector>
#include <glm/ext/matrix_float4x4.hpp>

#include "sgf/core/memory.h"
#include "sgf/assets/handle.h"
#include "material.h"

namespace sgf::assets
{
    struct Model final
    {
        struct Mesh final
        {
            // todo: impl
        };

        struct Node final
        {
            using allocator_type = std::pmr::polymorphic_allocator<>;

            core::String name;
            glm::mat4 local = glm::mat4(1.0f);
            std::optional<std::uint32_t> mesh;
            core::Vector<std::uint32_t> children;

            explicit Node(const allocator_type& alloc = {}) :
                name(alloc),
                children(alloc)
            { }
            Node(const Node&) = default;
            Node(const Node& other, const allocator_type& alloc = {}) :
                name(other.name, alloc),
                children(other.children, alloc)
            { }
            Node(Node&&) = default;
            Node(Node&& other, const allocator_type& alloc = {}) noexcept :
                name(std::move(other.name), alloc),
                children(std::move(other.children), alloc)
            { }

            [[nodiscard]] allocator_type get_allocator() const noexcept
            {
                return name.get_allocator();
            }

            Node& operator=(const Node&) = default;
            Node& operator=(Node&&) noexcept = default;
        };

        struct Scene final
        {
            using allocator_type = std::pmr::polymorphic_allocator<>;

            core::String name;
            core::Vector<std::uint32_t> roots;

            explicit Scene(const allocator_type& alloc = {}) :
                name(alloc),
                roots(alloc)
            { }
            Scene(const Scene&) = default;
            Scene(const Scene& other, const allocator_type& alloc = {}) :
                name(other.name, alloc),
                roots(other.roots, alloc)
            { }
            Scene(Scene&&) = default;
            Scene(Scene&& other, const allocator_type& alloc = {}) noexcept :
                name(std::move(other.name), alloc),
                roots(std::move(other.roots), alloc)
            { }

            [[nodiscard]] allocator_type get_allocator() const noexcept
            {
                return name.get_allocator();
            }

            Scene& operator=(const Scene&) = default;
            Scene& operator=(Scene&&) noexcept = default;
        };

        using allocator_type = std::pmr::polymorphic_allocator<>;

        core::Vector<Mesh> meshes;
        core::Vector<Node> nodes;
        core::Vector<Handle<Material>> materials;
        core::Vector<Scene> scenes;
        std::uint32_t default_scene {};

        explicit Model(const allocator_type& alloc = {}) :
            meshes(alloc),
            nodes(alloc),
            materials(alloc),
            scenes(alloc)
        { }
        Model(const Model&) = default;
        Model(const Model& other, const allocator_type& alloc) :
            meshes(other.meshes, alloc),
            nodes(other.nodes, alloc),
            materials(other.materials, alloc),
            scenes(other.scenes, alloc)
        { }
        Model(Model&&) noexcept = default;
        Model(Model&& other, const allocator_type& alloc) noexcept :
            meshes(std::move(other.meshes), alloc),
            nodes(std::move(other.nodes), alloc),
            materials(std::move(other.materials), alloc),
            scenes(std::move(other.scenes), alloc)
        { }

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return meshes.get_allocator();
        }

        Model& operator=(const Model&) = default;
        Model& operator=(Model&&) noexcept = default;
    };
}
