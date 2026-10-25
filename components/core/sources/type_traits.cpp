//
// Created by stuka on 18.10.2026.
//

#include "sgf/core/utility/type_traits.h"

#include <atomic>

std::uint32_t sgf::core::impl::nextIndex() noexcept
{
    static std::atomic<std::uint32_t> current {};
    return current++;
}