//
// Created by stuka on 14.10.2026.
//

#pragma once

#include <unordered_map>
#include <mutex>

#include "sgf/runtime/execution/thread_pool.h"

#include "system.h"
#include "system_graph.h"
#include "system_schedule.h"

namespace sgf::runtime
{
    struct SystemSchedulerCreateInfo final
    {
        using allocator_type = std::pmr::polymorphic_allocator<>;

        const SystemGraph* graph {};
        SystemContext* context {};
        core::Vector<SystemScheduleState> schedules;
        ThreadPool* thread_pool {};
        const core::LogCallback* debug_output {};

        SystemSchedulerCreateInfo() = default;
        SystemSchedulerCreateInfo(const allocator_type& alloc) noexcept :
            schedules(alloc)
        { }
        SystemSchedulerCreateInfo(const SystemSchedulerCreateInfo&) = default;
        SystemSchedulerCreateInfo(const SystemSchedulerCreateInfo& other, const allocator_type& alloc) :
            graph(other.graph),
            context(other.context),
            schedules(other.schedules, alloc),
            thread_pool(other.thread_pool),
            debug_output(other.debug_output)
        { }
        SystemSchedulerCreateInfo(SystemSchedulerCreateInfo&&) = default;
        SystemSchedulerCreateInfo(SystemSchedulerCreateInfo&& other, const allocator_type& alloc) noexcept :
            graph(other.graph),
            context(other.context),
            schedules(std::move(other.schedules), alloc),
            thread_pool(other.thread_pool),
            debug_output(other.debug_output)
        { }

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return schedules.get_allocator();
        }

        SystemSchedulerCreateInfo& operator=(const SystemSchedulerCreateInfo&) = default;
        SystemSchedulerCreateInfo& operator=(SystemSchedulerCreateInfo&&) noexcept = default;
    };

    struct SystemScheduler final
    {
        using allocator_type = std::pmr::polymorphic_allocator<>;

        SystemScheduler() noexcept = default;
        SystemScheduler(const allocator_type& alloc) noexcept :
            create_info_(alloc),
            executable_systems_(alloc),
            schedules_(alloc) { }
        SystemScheduler(const SystemScheduler& other) = delete;
        SystemScheduler(SystemScheduler&& other) = delete;

        ~SystemScheduler() noexcept;

        void run();
        void stop() noexcept;
        std::error_code runOnce();

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return executable_systems_.get_allocator();
        }

        SystemScheduler& operator=(const SystemScheduler& other) = delete;
        SystemScheduler& operator=(SystemScheduler&& other) = delete;

        static std::error_code create(SystemSchedulerCreateInfo create_info, SystemScheduler& output) noexcept;

    private:
        SystemSchedulerCreateInfo create_info_ {};

        std::mutex systems_access_;
        std::condition_variable systems_cv_;
        std::atomic<bool> running_;
        std::uint32_t running_systems_ {};
        core::Vector<std::uint32_t> executable_systems_;
        std::error_code last_error_{};
        core::Vector<SystemScheduleState> schedules_;
        std::optional<std::chrono::steady_clock::time_point> last_run_time_ {};

        void executeNode(SystemContext& context, SystemGraphState& graph_state, const SystemGraph::Node& node, bool ignore_schedule);
        std::error_code runImpl(bool ignore_schedule);
    };
}
