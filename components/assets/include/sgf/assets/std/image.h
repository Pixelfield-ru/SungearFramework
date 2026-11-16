//
// Created by stuka on 16.11.2026.
//

#pragma once

#include <cstdint>

#include "sgf/core/memory.h"

namespace sgf::assets
{
    /// 8-bit image. Pixels are tightly packed, row by row, `channels` components per texel.
    struct Image final
    {
        using allocator_type = std::pmr::polymorphic_allocator<>;

        std::uint32_t width {};
        std::uint32_t height {};
        /// 1, 2, 3 or 4.
        std::uint32_t channels {};
        core::Vector<std::byte> pixels;

        explicit Image(allocator_type allocator = {}) :
            pixels(allocator)
        { }

        Image(const Image& other, allocator_type allocator) :
            width(other.width),
            height(other.height),
            channels(other.channels),
            pixels(other.pixels, allocator)
        { }

        Image(Image&& other, allocator_type allocator) noexcept :
            width(other.width),
            height(other.height),
            channels(other.channels),
            pixels(std::move(other.pixels), allocator)
        { }

        Image(const Image&) = default;
        Image(Image&&) noexcept = default;
        Image& operator=(const Image&) = default;
        Image& operator=(Image&&) noexcept = default;
    };
}
