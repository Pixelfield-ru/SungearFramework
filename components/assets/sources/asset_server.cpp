//
// Created by stuka on 28.10.2026.
//

#include "sgf/assets/asset_server.h"

void sgf::assets::AssetServer::mount(std::filesystem::path path) noexcept
{
    catalog_.path = std::move(path);

}
