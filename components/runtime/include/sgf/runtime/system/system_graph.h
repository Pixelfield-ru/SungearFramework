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
        std::pmr::memory_resource* resource = std::pmr::get_default_resource();
    };

    struct SystemGraphState final
    {
        /// Indicates count of unfulfilled links for each node.
        core::Vector<std::uint32_t> remaining;
    };

    struct SystemGraph final
    {
        friend struct SystemScheduler;

        struct Node final
        {
            /// Executable system.
            ISystem* system {};
            /// Index in array.
            std::uint32_t index {};
            /// Indices of successors in array.
            core::Vector<std::uint32_t> successors;
            /// Count of links.
            std::uint32_t dependency_count {};
        };

        /// All nodes in graph.
        core::Vector<Node> nodes;
        /// Entry nodes.
        core::Vector<std::uint32_t> entries;

        [[nodiscard]] static std::expected<SystemGraph, std::error_code> build(const SystemGraphBuildInfo& build_info);
    };
}
