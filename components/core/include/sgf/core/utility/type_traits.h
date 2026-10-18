//
// Created by stuka on 14.10.2026.
//

#pragma once

#include <cstdint>

namespace sgf::core
{
    namespace impl
    {
        std::uint32_t& counter() noexcept;
    }

    /**
     * @return Sequenced index for type.
     */
    template<typename>
    std::uint32_t typeIndex() noexcept
    {
        static std::uint32_t index = []() {
            auto& current = impl::counter();
            const auto idx = current;
            ++current;
            return idx;
        }();

        return index;
    }
}