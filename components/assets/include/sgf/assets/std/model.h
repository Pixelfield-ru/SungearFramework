//
// Created by stuka on 18.11.2026.
//

#pragma once

#include <vector>

namespace sgf::assets
{
    struct Model final
    {
        using allocator_type = std::pmr::polymorphic_allocator<>;

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return {};
        }

    private:
    };
}