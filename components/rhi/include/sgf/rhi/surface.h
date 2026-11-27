//
// Created by stuka on 17.11.2026.
//

#pragma once

#include <system_error>
#include <utility>

#include <glm/vec2.hpp>

#include "sgf/core/memory.h"
#include "types.h"

namespace sgf::platform
{
    struct Window;
}

namespace sgf::rhi
{
    struct IContext;

    struct SurfaceFormat final
    {
        Format format = Format::kUnknown;
        ColorSpace color_space = ColorSpace::kUnknown;
    };

    struct SurfaceCapabilities final
    {
        using allocator_type = std::pmr::polymorphic_allocator<>;

        core::Vector<QueueFamilyType> present_queue_families;
        core::Vector<SurfaceFormat> formats;
        core::Vector<PresentMode> present_modes;
        glm::uvec2 current_extent {};
        glm::uvec2 min_extent {};
        glm::uvec2 max_extent {};
        SurfaceTransform current_transform = SurfaceTransform::kIdentity;
        std::uint32_t min_image_count {};
        std::uint32_t max_image_count {};

        explicit SurfaceCapabilities(const allocator_type& allocator = {}) :
            present_queue_families(allocator),
            formats(allocator),
            present_modes(allocator)
        { }
        SurfaceCapabilities(const SurfaceCapabilities&) = default;
        SurfaceCapabilities(const SurfaceCapabilities& other, const allocator_type& allocator) :
            present_queue_families(other.present_queue_families, allocator),
            formats(other.formats, allocator),
            present_modes(other.present_modes, allocator),
            current_extent(other.current_extent),
            min_extent(other.min_extent),
            max_extent(other.max_extent),
            current_transform(other.current_transform),
            min_image_count(other.min_image_count),
            max_image_count(other.max_image_count)
        { }
        SurfaceCapabilities(SurfaceCapabilities&&) noexcept = default;
        SurfaceCapabilities(SurfaceCapabilities&& other, const allocator_type& allocator) :
            present_queue_families(std::move(other.present_queue_families), allocator),
            formats(std::move(other.formats), allocator),
            present_modes(std::move(other.present_modes), allocator),
            current_extent(other.current_extent),
            min_extent(other.min_extent),
            max_extent(other.max_extent),
            current_transform(other.current_transform),
            min_image_count(other.min_image_count),
            max_image_count(other.max_image_count)
        { }

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return formats.get_allocator();
        }

        SurfaceCapabilities& operator=(const SurfaceCapabilities&) = default;
        SurfaceCapabilities& operator=(SurfaceCapabilities&&) noexcept = default;
    };

    struct SurfaceCreateInfo final
    {
        const IContext* context {};
        const platform::Window* window {};
    };

    struct ISurface
    {
        virtual ~ISurface() = default;

    private:
        virtual void destroy() = 0;
        [[nodiscard]] virtual std::error_code create(const SurfaceCreateInfo& create_info) = 0;
    };
}