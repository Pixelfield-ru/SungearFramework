//
// Created by stuka on 14.10.2026.
//

#pragma once

#include <cstdint>
#include <type_traits>

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

    template<typename>
    struct AlwaysFalse : std::false_type { };

    template<typename T>
    static constexpr bool always_false_v = AlwaysFalse<T>::value;
}
