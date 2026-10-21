//
// Created by stuka on 03.09.2026.
//

#pragma once

#include <string>

#include "sgf/core/memory.h"
#include "types.h"

namespace sgf::rhi
{
    struct ShaderStage
    {
        ShaderStageType type = ShaderStageType::kUnknown;
        core::String entry_point_name;
    };
}