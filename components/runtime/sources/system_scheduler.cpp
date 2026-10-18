//
// Created by stuka on 14.10.2026.
//

#include "sgf/runtime/system/system_scheduler.h"

#include "sgf/runtime/system/system_graph.h"

// todo: review

sgf::runtime::SystemScheduler::SystemScheduler(SystemSchedulerCreateInfo create_info) noexcept
{
    create_info_ = std::move(create_info);
}

sgf::runtime::SystemScheduler::~SystemScheduler() noexcept
{
    stop();
}

void sgf::runtime::SystemScheduler::run() noexcept
{
    running_ = true;

    while(running_)
    {
        if(const auto execution_res = runOnce())
        {
            if(create_info_.debug_output)
            {
                (*create_info_.debug_output)(core::LogLevel::kError,
                                             std::format("Unexpected error while running scheduler: {}",
                                                         execution_res.message()));
            }
        }
    }
}

void sgf::runtime::SystemScheduler::stop() noexcept
{
    running_ = false;
}

std::error_code sgf::runtime::SystemScheduler::runOnce() noexcept
{
    last_error_ = {};

    const auto& graph = *create_info_.graph;
    auto& context = *create_info_.context;

    SystemGraphState state;
    state.remaining.resize(graph.nodes.size());
    for(const auto& node : graph.nodes)
    {
        state.remaining[node.index] = node.dependency_count;
    }

    executable_systems_ = graph.entries;

    std::unique_lock lock(systems_access_);
    while(last_error_ == std::error_code { } &&
          (!executable_systems_.empty() || running_systems_ > 0))
    {
        if(executable_systems_.empty())
        {
            systems_cv_.wait(lock);
            continue;
        }

        auto& node = graph.nodes[executable_systems_.back()];
        executable_systems_.pop_back();
        ++running_systems_;
        lock.unlock();

        executeNode(context, state, node);

        lock.lock();
    }

    // if caught error then previous cycle was interrupted so we need to wait all systems
    while(running_systems_ > 0)
    {
        systems_cv_.wait(lock);
    }

    return last_error_;
}

void sgf::runtime::SystemScheduler::executeNode(SystemContext& context, SystemGraphState& graph_state,
                                               const SystemGraph::Node& node) noexcept
{
    auto* system = node.system;

    auto execution = [this, system, &context, &node, &graph_state]() {
        const auto res = system->execute(context);

        std::lock_guard guard(systems_access_);

        if(res && !last_error_) last_error_ = res;

        for(const auto successor_index : node.successors)
        {
            auto& successor_remaining = graph_state.remaining[successor_index];
            if(--successor_remaining > 0) continue;

            executable_systems_.push_back(successor_index);
        }

        --running_systems_;
        systems_cv_.notify_one();
    };

    if(system->executionDomain() == ExecutionDomain::kAny && create_info_.thread_pool)
    {
        create_info_.thread_pool->post(std::move(execution));
    }
    else
    {
        execution();
    }
}
