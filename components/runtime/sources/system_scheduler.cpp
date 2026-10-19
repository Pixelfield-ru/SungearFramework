//
// Created by stuka on 14.10.2026.
//

#include "sgf/runtime/system/system_scheduler.h"

#include "sgf/runtime/errors.h"
#include "sgf/runtime/system/system_graph.h"

// todo: review
// todo: fix run time

std::error_code sgf::runtime::SystemScheduler::create(SystemSchedulerCreateInfo create_info,
    SystemScheduler& output) noexcept
{
    output.create_info_ = std::move(create_info);

    output.schedules_.resize(SystemScheduleType::kCount);
    output.schedules_state_.resize(SystemScheduleType::kCount);

    // creating schedules
    for(size_t i = 0; i < SystemScheduleType::kCount; ++i)
    {
        output.schedules_[i].type = static_cast<SystemScheduleType>(i);
        output.schedules_state_[i].schedule_type = static_cast<SystemScheduleType>(i);
    }

    // filling schedules info
    for(const auto& schedule : output.create_info_.schedules)
    {
        if(schedule.interval == std::chrono::nanoseconds::zero())
        {
            if(output.create_info_.debug_output)
            {
                (*output.create_info_.debug_output)(core::LogLevel::kError,
                                                    std::format("Schedule {} has zero interval",
                                                                std::to_underlying(schedule.type)));
            }
            return Error::kInvalidScheduleInterval;
        }

        output.schedules_[schedule.type] = schedule;
        output.schedules_state_[schedule.type].schedule_type = schedule.type;
    }

    // verifying systems schedules
    for(const auto& node : output.create_info_.graph->nodes)
    {
        const auto* system = node.system;

        const auto& schedule = output.schedules_[system->schedule()];
        if(schedule.interval == std::chrono::nanoseconds::zero())
        {
            if(output.create_info_.debug_output)
            {
                (*output.create_info_.debug_output)(core::LogLevel::kError,
                                                    std::format("Schedule {} has zero interval",
                                                                std::to_underlying(schedule.type)));
            }
            return Error::kInvalidScheduleInterval;
        }
    }

    return {};
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

    const auto current_run_time = std::chrono::steady_clock::now();
    bool first_time {};
    if(!last_run_time_)
    {
        last_run_time_ = current_run_time;
        first_time = true;
    }
    const auto run_time_delta = current_run_time - *last_run_time_;
    last_run_time_ = current_run_time;

    for(auto& schedule_state : schedules_state_)
    {
        schedule_state.accumulator += first_time ? schedules_[schedule_state.schedule_type].interval : run_time_delta;
    }

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

    for(auto& schedule_state : schedules_state_)
    {
        const auto& schedule = schedules_[schedule_state.schedule_type];
        const auto interval = schedule.interval;

        if(interval == std::chrono::nanoseconds::zero()) continue;
        if(schedule_state.accumulator >= interval)
        {
            const auto repeat_times = std::min<size_t>(schedule_state.accumulator / interval, schedule.max_catch_up_steps);
            schedule_state.accumulator = schedule_state.accumulator - (interval * repeat_times);
        }
    }

    return last_error_;
}

void sgf::runtime::SystemScheduler::executeNode(SystemContext& context, SystemGraphState& graph_state,
                                                const SystemGraph::Node& node) noexcept
{
    auto* system = node.system;

    const auto schedule_type = system->schedule();
    const auto schedule_interval = schedules_[schedule_type].interval;
    const auto max_catch_up_steps = schedules_[schedule_type].max_catch_up_steps;

    auto notify_successors = [this, &node, &graph_state]() {
        for(const auto successor_index : node.successors)
        {
            auto& successor_remaining = graph_state.remaining[successor_index];
            if(--successor_remaining > 0) continue;

            executable_systems_.push_back(successor_index);
        }

        --running_systems_;
        systems_cv_.notify_one();
    };

    auto execution = [this, system, &context, notify_successors, schedule_type, schedule_interval, max_catch_up_steps] () {
        std::error_code execute_res {};

        const auto repeat_times = schedules_state_[schedule_type].accumulator / schedule_interval;
        for(size_t i = 0; i < repeat_times; ++i)
        {
            if(i >= max_catch_up_steps) break;

            execute_res = system->execute(context);
            if(execute_res) break;
        }

        std::lock_guard guard(systems_access_);

        if(execute_res && !last_error_) last_error_ = execute_res;

        notify_successors();
    };

    if(schedules_state_[schedule_type].accumulator < schedule_interval)
    {
        std::lock_guard guard(systems_access_);

        notify_successors();
        return;
    }

    if(system->executionDomain() == ExecutionDomain::kAny && create_info_.thread_pool)
    {
        create_info_.thread_pool->post(std::move(execution));
    }
    else
    {
        execution();
    }
}
