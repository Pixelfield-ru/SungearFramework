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
        const SystemGraph* graph {};
        SystemContext* context;
        std::vector<SystemScheduleState> schedules;
        ThreadPool* thread_pool {};
        const core::LogCallback* debug_output {};
    };

    struct SystemScheduler final
    {
        SystemScheduler() noexcept = default;
        SystemScheduler(const SystemScheduler&) noexcept = delete;
        SystemScheduler(SystemScheduler&&) noexcept = delete;
        ~SystemScheduler() noexcept;

        void run();
        void stop() noexcept;
        std::error_code runOnce();

        static std::error_code create(SystemSchedulerCreateInfo create_info, SystemScheduler& output) noexcept;

    private:
        SystemSchedulerCreateInfo create_info_ {};

        std::mutex systems_access_;
        std::condition_variable systems_cv_;
        std::atomic<bool> running_;
        std::uint32_t running_systems_ {};
        std::vector<std::uint32_t> executable_systems_;
        std::error_code last_error_{};
        std::vector<SystemScheduleState> schedules_;
        std::optional<std::chrono::steady_clock::time_point> last_run_time_ {};

        void executeNode(SystemContext& context, SystemGraphState& graph_state, const SystemGraph::Node& node, bool ignore_schedule);
        std::error_code runImpl(bool ignore_schedule);
    };
}
