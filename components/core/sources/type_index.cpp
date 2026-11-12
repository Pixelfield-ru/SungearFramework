//
// Created by stuka on 18.10.2026.
//

#include "sgf/core/utility/type_index.h"

#include <atomic>

std::uint32_t sgf::core::impl::nextIndex() noexcept
{
    static std::atomic<std::uint32_t> current {};
    return current++;
}

std::unordered_map<std::uint32_t, sgf::core::impl::StableTypeRecord>& sgf::core::impl::stableTypes()
{
    static std::unordered_map<std::uint32_t, StableTypeRecord> types;
    return types;
}

std::uint32_t sgf::core::typeIndexFromStableHash(std::uint32_t stable_hash)
{
    std::shared_lock lock(impl::stableTypesMutex());

    const auto it = impl::stableTypes().find(stable_hash);
    SGF_ASSERT(it != impl::stableTypes().end(), std::format("Unregistered stable type hash: {}", stable_hash).c_str());

    return it->second.type_index;
}
