//
// Created by stuka on 14.10.2026.
//

#include "sgf/runtime/system/system_graph.h"

#include <algorithm>
#include <ranges>
#include <unordered_set>

#include "sgf/runtime/system/system.h"
#include "sgf/runtime/errors.h"
#include "sgf/core/errors.h"

std::expected<sgf::runtime::SystemGraph, std::error_code> sgf::runtime::SystemGraph::build(
    const SystemGraphBuildInfo& build_info)
{
    // todo: maybe needs optimization
    if(!build_info.context) return std::unexpected(core::Error::kInvalidArgument);
    if(!build_info.debug_output) return std::unexpected(core::Error::kInvalidArgument);

    auto* logger = build_info.debug_output;

    SystemGraph graph;
    graph.nodes = core::Vector<Node>(build_info.systems.size(), build_info.memory_resource);
    graph.entries = core::Vector<std::uint32_t>(build_info.systems.size(), build_info.memory_resource);

    const auto& context = build_info.context;

    const auto validate_access = [&](ISystem& system,
                                     std::span<const std::uint32_t> types,
                                     std::string_view access) {
        for(const auto type : types)
        {
            if(context->resources.hasType(type)) continue;
            if(logger)
            {
                (*logger)(core::LogLevel::kError,
                          std::format("System '{}' {} missing type {}.",
                                      typeid(system).name(), access, type));
            }
            return false;
        }
        return true;
    };

    // adding nodes
    for(size_t i = 0; i < build_info.systems.size(); ++i)
    {
        auto* system = build_info.systems[i];

        // validation
        const auto system_reads = system->reads();
        if(!validate_access(*system, system_reads, "reads"))
        {
            return std::unexpected(Error::kIncompleteSystemContext);
        }

        const auto system_writes = system->writes();
        if(!validate_access(*system, system_writes, "writes"))
        {
            return std::unexpected(Error::kIncompleteSystemContext);
        }

        Node node;
        node.system = system;
        node.index = i;
        node.successors = core::Vector<std::uint32_t>(build_info.memory_resource);

        graph.nodes[i] = std::move(node);
    }

    core::UnorderedSet<std::uint32_t> successors_set;

    // building dependencies
    for(auto& node : graph.nodes)
    {
        const auto system_writes = node.system->writes();

        for(const auto write_id : system_writes)
        {
            for(auto& other : graph.nodes)
            {
                if(other.index == node.index) continue;

                const auto other_sys_reads = other.system->reads();
                const auto other_sys_writes = other.system->writes();

                const bool other_reads =
                    std::ranges::contains(other_sys_reads, write_id);
                const bool other_writes =
                    std::ranges::contains(other_sys_writes, write_id);

                const bool writer_to_reader =
                    other_reads && !other_writes;
                const bool writer_to_writer =
                    other_writes && node.index < other.index;

                // no edge (nodes dont depend on each other)
                if (!writer_to_reader && !writer_to_writer)
                    continue;

                // if current node has this successor then skipping the successor
                if(!successors_set.insert(other.index).second)
                    continue;

                ++other.dependency_count;
                node.successors.push_back(other.index);
            }
        }

        successors_set.clear();
    }

    // getting entries
    for(size_t i = 0; i < graph.nodes.size(); ++i)
    {
        const auto& node = graph.nodes[i];
        if(node.dependency_count > 0) continue;

        graph.entries[i] = node.index;
    }

    // initializing state to check cycles
    SystemGraphState state;
    state.remaining = core::Vector<std::uint32_t>(graph.nodes.size());
    for(const auto& node : graph.nodes)
    {
        state.remaining[node.index] = node.dependency_count;
    }

    // check for cycles
    auto executable_systems = graph.entries;
    while(!executable_systems.empty())
    {
        const auto& node = graph.nodes[executable_systems.back()];
        executable_systems.pop_back();

        for(const auto successor_index : node.successors)
        {
            auto& successor_remaining = state.remaining[successor_index];
            --successor_remaining;
            if(successor_remaining > 0) continue;

            executable_systems.push_back(successor_index);
        }
    }

    // check if any node has remaining edges
    for(const auto& node : graph.nodes)
    {
        const auto remaining = state.remaining[node.index];
        if(remaining == 0) continue;

        if(logger)
        {
            (*logger)(core::LogLevel::kError,
                      std::format("Graph is cyclic. System {} has {} unfulfilled edges left.",
                                  typeid(*node.system).name(), remaining));
        }

        return std::unexpected(Error::kCyclicSystemGraph);
    }


    return graph;
}
