//
// Created by stuka on 21.10.2026.
//

#pragma once

#include <memory>
#include <memory_resource>
#include <unordered_set>
#include <unordered_map>

namespace sgf::core
{
    template<typename T>
    struct MemoryResourceDeleter
    {
        std::pmr::memory_resource* resource { };
        void* object { };

        void (*destroy)(std::pmr::memory_resource*, void*) { };

        MemoryResourceDeleter() = default;

        MemoryResourceDeleter(std::pmr::memory_resource* resource, T* object) : resource { resource },
            object { object },
            destroy { &destroy_as<T> }
        {
        }

        template<typename U>
        requires std::convertible_to<U*, T*>
        MemoryResourceDeleter(const MemoryResourceDeleter<U>& other) noexcept : resource { other.resource },
            object { other.object },
            destroy { other.destroy }
        {
        }

        void operator()(T*) const { destroy(resource, object); }

        template<typename U>
        static void destroy_as(std::pmr::memory_resource* resource, void* object)
        {
            std::pmr::polymorphic_allocator<U> { resource }.delete_object(static_cast<U*>(object));
        }
    };

    template<typename T>
    using UniquePtr = std::unique_ptr<T, MemoryResourceDeleter<T>>;

    template<typename T, typename... Args>
    UniquePtr<T> make_unique(std::pmr::memory_resource* resource, Args&&... args)
    {
        std::pmr::polymorphic_allocator<T> allocator { resource };
        T* object = allocator.template new_object<T>(std::forward<Args>(args)...);

        return UniquePtr<T> {
            object,
            MemoryResourceDeleter<T> { resource, object }
        };
    }

    template<typename T>
    using SharedPtr = std::shared_ptr<T>;

    template<typename T, typename... Args>
    SharedPtr<T> make_shared(std::pmr::memory_resource* resource, Args&&... args)
    {
        std::pmr::polymorphic_allocator<T> allocator { resource };

        return std::allocate_shared<T>(allocator, std::forward<Args>(args)...);
    }

    template<typename T>
    using Vector = std::pmr::vector<T>;

    template<typename T, typename Hasher = std::hash<T>, class Eq = std::equal_to<T>>
    using UnorderedSet = std::pmr::unordered_set<T, Hasher, Eq>;

    template <typename KeyT, typename ValueT, typename Hasher = std::hash<KeyT>, typename KeyEq = std::equal_to<KeyT>>
    using UnorderedMap = std::pmr::unordered_map<KeyT, ValueT, Hasher, KeyEq>;

    template<typename CharT>
    using BasicString = std::pmr::basic_string<CharT>;
    using String = std::pmr::string;
    using U32String = std::pmr::basic_string<char32_t>;
}
