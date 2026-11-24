//
// Created by stuka on 17.10.2026.
//

#pragma once

#include <cstdint>

namespace sgf::core
{
    enum class ExecutionDomain : std::uint8_t
    {
        /// Runs on the calling thread.
        kCurrent,
        /// Runs in any thread.
        kAny
    };
}
