//
// Created by stuka on 12.11.2026.
//

#pragma once

#include <cstdint>

namespace sgf::platform
{
    /// Dense codes for a state array. Backend button numbers are mapped into these.
    enum class MouseButton : std::uint8_t
    {
        kLeft = 0,
        kRight,
        kMiddle,
        kButton4,
        kButton5,
        kButton6,
        kButton7,
        kButton8,

        kCount
    };

    // todo:
    struct Mouse final
    {

    };
}
