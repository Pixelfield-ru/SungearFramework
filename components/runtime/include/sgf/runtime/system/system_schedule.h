//
// Created by stuka on 19.10.2026.
//

#pragma once

#include <cstdint>
#include <chrono>

namespace sgf::runtime
{
    enum SystemScheduleType : std::uint8_t
    {
        kFrame,
        kFixed,
        kRender,
        kSimulation,
        kBackground,
        kAudio,
        kCount
    };

    struct SystemScheduleState final
    {
        SystemScheduleType type = SystemScheduleType::kFrame;
        std::chrono::nanoseconds interval {};
        std::uint32_t max_catch_up_steps = 1;
        std::chrono::steady_clock::duration accumulator {};
        size_t steps {};
    };
}