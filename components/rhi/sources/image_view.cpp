//
// Created by stuka on 11.10.2026.
//

#include "sgf/rhi/image_view.h"

std::error_code sgf::rhi::IImageView::recreate(const ImageViewCreateInfo& create_info) noexcept
{
    destroy();
    return create(create_info);
}
