//
// Created by stuka on 14.10.2026.
//

#pragma once

#include <expected>
#include <system_error>
#include <vector>
#include <span>
#include <memory_resource>

#include "sgf/core/log.h"
#include "sgf/core/memory.h"

namespace sgf::runtime
{
    struct ISystem;
    struct SystemContext;

    struct SystemGraphBuildInfo final
    {
        std::span<ISystem *const> systems;
        const SystemContext* context {};
        const core::LogCallback* debug_output {};
    };

    struct SystemGraphState final
    {
        using allocator_type = std::pmr::polymorphic_allocator<>;

        /// Indicates count of unfulfilled links for each node.
        core::Vector<std::uint32_t> remaining;

        SystemGraphState(const allocator_type& alloc) noexcept :
            remaining(alloc)
        { }
        SystemGraphState() noexcept = default;
        SystemGraphState(const SystemGraphState& other) noexcept = default;
        SystemGraphState(const SystemGraphState& other, const allocator_type& alloc) :
           remaining(other.remaining, alloc)
        { }
        SystemGraphState(SystemGraphState&&) noexcept = default;
        SystemGraphState(SystemGraphState&& other, const allocator_type& alloc) :
            remaining(std::move(other.remaining), alloc)
        { }

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return remaining.get_allocator();
        }

        SystemGraphState& operator=(const SystemGraphState&) = default;
        SystemGraphState& operator=(SystemGraphState&&) noexcept = default;
    };

    struct SystemGraph final
    {
        friend struct SystemScheduler;

        using allocator_type = std::pmr::polymorphic_allocator<>;

        struct Node final
        {
            using allocator_type = std::pmr::polymorphic_allocator<>;

            /// Executable system.
            ISystem* system {};
            /// Index in array.
            std::uint32_t index {};
            /// Indices of successors in array.
            core::Vector<std::uint32_t> successors;
            /// Count of links.
            std::uint32_t dependency_count {};

            Node(const allocator_type& alloc) noexcept :
                successors(alloc)
            { }
            Node() noexcept = default;
            Node(const Node& other) noexcept = default;
            Node(const Node& other, const allocator_type& alloc) :
                system(other.system),
                index(other.index),
                successors(other.successors, alloc),
                dependency_count(other.dependency_count)
            { }
            Node(Node&&) noexcept = default;
            Node(Node&& other, const allocator_type& alloc) :
                system(other.system),
                index(other.index),
                successors(std::move(other.successors), alloc),
                dependency_count(other.dependency_count)
            { }

            [[nodiscard]] allocator_type get_allocator() const noexcept
            {
                return successors.get_allocator();
            }

            Node& operator=(const Node&) = default;
            Node& operator=(Node&&) noexcept = default;
        };

        /// All nodes in graph.
        core::Vector<Node> nodes;
        /// Entry nodes.
        core::Vector<std::uint32_t> entries;

        SystemGraph() = default;
        SystemGraph(const allocator_type& alloc) :
            nodes(alloc),
            entries(alloc)
        { }
        SystemGraph(const SystemGraph&) = default;
        SystemGraph(const SystemGraph& other, const allocator_type& alloc) :
            nodes(other.nodes, alloc),
            entries(other.entries, alloc)
        { }
        SystemGraph(SystemGraph&&) = default;
        SystemGraph(SystemGraph&& other, const allocator_type& alloc) :
            nodes(std::move(other.nodes), alloc),
            entries(std::move(other.entries), alloc)
        { }

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return nodes.get_allocator();
        }

        SystemGraph& operator=(const SystemGraph&) = default;
        SystemGraph& operator=(SystemGraph&&) noexcept = default;

        [[nodiscard]] static std::error_code build(const SystemGraphBuildInfo& build_info, SystemGraph& output);
    };
}
