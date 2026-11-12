//
// Created by stuka on 12.11.2026.
//

#pragma once

#include <cstddef>

#include "sgf/core/memory.h"

namespace sgf::assets
{
    /// Whole contents of a file.
    struct Blob final
    {
        using allocator_type = std::pmr::polymorphic_allocator<>;

        core::Vector<std::byte> bytes;

        Blob() = default;

        explicit Blob(allocator_type allocator) :
            bytes(allocator)
        { }

        Blob(const Blob& other, allocator_type allocator) :
            bytes(other.bytes, allocator)
        { }

        Blob(Blob&& other, allocator_type allocator) noexcept :
            bytes(std::move(other.bytes), allocator)
        { }

        Blob(const Blob&) = default;
        Blob(Blob&&) noexcept = default;
        Blob& operator=(const Blob&) = default;
        Blob& operator=(Blob&&) noexcept = default;
    };
}
