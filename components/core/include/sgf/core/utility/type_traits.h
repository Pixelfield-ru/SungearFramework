//
// Created by stuka on 14.10.2026.
//

#pragma once

#include <cstdint>

namespace sgf::core
{
    namespace impl
    {
        std::uint32_t nextIndex() noexcept;
    }

    /**
     * @return Sequenced index for type.
     */
    template<typename>
    std::uint32_t typeIndex() noexcept
    {
        static const auto index = impl::nextIndex();

        return index;
    }
}