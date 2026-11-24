//
// Created by stuka on 16.11.2026.
//

#pragma once

#include <cstdint>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "sgf/assets/handle.h"
#include "sgf/assets/std/image.h"

namespace sgf::assets
{
    struct Material final
    {
        enum class AlphaMode : std::uint8_t
        {
            kOpaque,
            kMask,
            kBlend
        };

        glm::vec4 base_color = glm::vec4(1);
        float metallic {};
        float roughness {};
        glm::vec3 emissive = glm::vec3(1.0f);
        float alpha_cutoff {};
        bool double_sided {};
        AlphaMode alpha_mode = AlphaMode::kOpaque;

        /// Empty when that glTF slot has no image.
        Handle<Image> base_color_image;
        Handle<Image> metallic_roughness_image;
        Handle<Image> normal_image;
        Handle<Image> occlusion_image;
        Handle<Image> emissive_image;
    };
}