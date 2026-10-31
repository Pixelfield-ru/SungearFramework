//
// Created by stuka on 31.10.2026.
//

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <memory_resource>

namespace sgf::serialization
{
    // RapidJSON releases memory through static Free(void*). pmr::deallocate also needs the size and the resource.
    // private header.
    struct RapidJsonPmrAllocator
    {
        static const bool kNeedFree = true;
        static constexpr std::size_t alignment = alignof(std::max_align_t);

        struct Header
        {
            std::pmr::memory_resource* resource {};
            std::size_t bytes {};
        };

        static constexpr std::size_t header_bytes = (sizeof(Header) + alignment - 1) & ~(alignment - 1);

        std::pmr::memory_resource* resource {};

        void* Malloc(const std::size_t size) const
        {
            if(size == 0) return nullptr;

            const std::size_t bytes = header_bytes + size;
            auto* raw = static_cast<std::byte*>(resource->allocate(bytes, alignment));
            auto* header = reinterpret_cast<Header*>(raw);
            header->resource = resource;
            header->bytes = bytes;
            return raw + header_bytes;
        }

        void* Realloc(void* original, std::size_t original_size, std::size_t new_size)
        {
            if(new_size == 0)
            {
                Free(original);
                return nullptr;
            }

            if(original == nullptr) return Malloc(new_size);

            void* next = Malloc(new_size);
            std::memcpy(next, original, std::min(original_size, new_size));
            Free(original);
            return next;
        }

        static void Free(void* pointer) noexcept
        {
            if(pointer == nullptr) return;

            auto* raw = static_cast<std::byte*>(pointer) - header_bytes;
            const auto* header = reinterpret_cast<const Header*>(raw);
            header->resource->deallocate(raw, header->bytes, alignment);
        }

        bool operator==(const RapidJsonPmrAllocator& other) const noexcept
        {
            return resource == other.resource;
        }

        bool operator!=(const RapidJsonPmrAllocator& other) const noexcept
        {
            return resource != other.resource;
        }
    };
}
