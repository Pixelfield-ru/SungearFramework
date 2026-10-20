//
// Created by stuka on 14.10.2026.
//

#include "sgf/runtime/system/system_scheduler.h"

#include "sgf/runtime/errors.h"
#include "sgf/runtime/system/system_graph.h"

// todo: review

std::error_code sgf::runtime::SystemScheduler::create(SystemSchedulerCreateInfo create_info,
    SystemScheduler& output) noexcept
{
    output.create_info_ = std::move(create_info);

    output.schedules_.resize(SystemScheduleType::kCount);

    // creating schedules
    for(size_t i = 0; i < SystemScheduleType::kCount; ++i)
    {
        output.schedules_[i].type = static_cast<SystemScheduleType>(i);
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

void sgf::runtime::SystemScheduler::run()
{
    bool first_time = !running_;
    running_ = true;

    while(running_)
    {
        if(const auto execution_res = runImpl(first_time))
        {
            if(create_info_.debug_output)
            {
                (*create_info_.debug_output)(core::LogLevel::kError,
                                             std::format("Unexpected error while running scheduler: {}",
                                                         execution_res.message()));
            }
        }

        first_time = false;
    }
}

void sgf::runtime::SystemScheduler::stop() noexcept
{
    running_ = false;
}

std::error_code sgf::runtime::SystemScheduler::runOnce()
{
    // ignoring schedule to run all systems
    return runImpl(true);
}

void sgf::runtime::SystemScheduler::executeNode(SystemContext& context, SystemGraphState& graph_state,
                                                const SystemGraph::Node& node, bool ignore_schedule)
{
    auto* system = node.system;

    const auto schedule_type = system->schedule();
    const auto steps_count = ignore_schedule ? 1 : schedules_[schedule_type].steps;

    auto notify_successors = [this, &node, &graph_state]() {
        for(const auto successor_index : node.successors)
        {
            auto& successor_remaining = graph_state.remaining[successor_index];
            if(--successor_remaining > 0) continue;

            // pushing new systems to execute
            executable_systems_.push_back(successor_index);
        }

        --running_systems_;
        // notifying cv to check if there are any new systems to execute
        systems_cv_.notify_one();
    };

    auto execution = [this, system, &context, notify_successors, steps_count] {
        std::error_code execute_res {};


        for(size_t i = 0; i < steps_count; ++i)
        {
            execute_res = system->execute(context);
            if(execute_res) break;
        }

        std::lock_guard guard(systems_access_);

        if(execute_res && !last_error_) last_error_ = execute_res;

        notify_successors();
    };

    // if schedule`s time did not come then skipping this node and notifying successors
    if(!ignore_schedule && steps_count == 0)
    {
        notify_successors();
        return;
    }

    // if can be executed parallel
    if(system->executionDomain() == ExecutionDomain::kAny && create_info_.thread_pool)
    {
        create_info_.thread_pool->post(std::move(execution));
    }
    else
    {
        execution();
    }
}

std::error_code sgf::runtime::SystemScheduler::runImpl(bool ignore_schedule)
{
    last_error_ = {};

    const auto current_run_time = std::chrono::steady_clock::now();
    if(!last_run_time_)
    {
        last_run_time_ = current_run_time;
    }
    const auto run_time_delta = current_run_time - *last_run_time_;
    last_run_time_ = current_run_time;

    if(!ignore_schedule)
    {
        // calculating statistics for the schedule
        for(auto& schedule : schedules_)
        {
            schedule.accumulator += run_time_delta;

            // skipping invalid schedule
            if(schedule.interval == std::chrono::nanoseconds::zero())
            {
                schedule.steps = 0;
                continue;
            }

            schedule.steps = std::min<size_t>(schedule.accumulator / schedule.interval, schedule.max_catch_up_steps);
        }
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
        const auto& schedule = schedules_[node.system->schedule()];
        executable_systems_.pop_back();
        ++running_systems_;

        // if schedule is waiting then we can keep mutex locked
        if(ignore_schedule || schedule.steps > 0)
        {
            lock.unlock();
        }

        executeNode(context, state, node, ignore_schedule);

        if(ignore_schedule || schedule.steps > 0)
        {
            lock.lock();
        }
    }

    // if caught error then previous cycle was interrupted so we need to wait all systems
    while(running_systems_ > 0)
    {
        systems_cv_.wait(lock);
    }

    if(!ignore_schedule)
    {
        for(auto& schedule : schedules_)
        {
            const auto interval = schedule.interval;

            // calculating new accumulator after executing systems in the schedule
            if(schedule.accumulator >= interval)
            {
                schedule.accumulator = schedule.accumulator - (interval * schedule.steps);
            }
        }
    }

    return last_error_;
}
