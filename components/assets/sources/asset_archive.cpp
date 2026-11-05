//
// Created by stuka on 05.11.2026.
//

#include "sgf/assets/asset_archive.h"

sgf::assets::Archive::~Archive()
{
    std::ignore = close();
}

std::error_code sgf::assets::Archive::open()
{
    return {};
}

std::error_code sgf::assets::Archive::close()
{
    return {};
}

std::filesystem::path sgf::assets::Archive::path() const noexcept
{
    return path_;
}
