//
// Created by stuka on 12.10.2026.
//

#pragma once

#include <string>
#include <filesystem>

#include "sgf/core/memory.h"

namespace sgf::core
{
    String pathToUtf8(const std::filesystem::path& p) noexcept;
}