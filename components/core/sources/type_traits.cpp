//
// Created by stuka on 18.10.2026.
//

#include "sgf/core/utility/type_traits.h"

std::uint32_t& sgf::core::impl::counter() noexcept
{
    static std::uint32_t current {};
    return current;
}