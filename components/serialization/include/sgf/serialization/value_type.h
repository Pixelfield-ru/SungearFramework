//
// Created by stuka on 02.11.2026.
//

#pragma once

#include <cstdint>

namespace sgf::serialization
{
    enum class ValueType : std::int8_t
    {
        kDirect,
        kObject,
        kArray
    };
}