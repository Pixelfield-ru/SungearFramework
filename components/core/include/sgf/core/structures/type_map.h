//
// Created by stuka on 14.10.2026.
//

#pragma once

#include <memory>
#include <vector>

#include "sgf/core/utility/type_traits.h"
#include "sgf/core/utility/assert.h"
#include "sgf/core/memory.h"

namespace sgf::core
{
    struct TypeMap final
    {
        using allocator_type = std::pmr::polymorphic_allocator<TypeMap>;

        TypeMap(allocator_type alloc) noexcept :
            storage_(alloc)
        {

        }
        TypeMap() noexcept = default;
        TypeMap(const TypeMap&) = delete;
        TypeMap(const TypeMap& other, allocator_type alloc) = delete;
        TypeMap(TypeMap&&) = default;
        TypeMap(TypeMap&& other, allocator_type alloc) :
            storage_(std::move(other.storage_), alloc)
        {

        }

        template<typename T, typename... Args>
        T& emplace(Args&&... args) noexcept
        {
            static const auto index = typeIndex<T>();
            if(index >= storage_.size())
            {
                storage_.resize(index + 1);
            }

            auto storage = core::make_unique<Storage<T>>(storage_.get_allocator().resource(), std::forward<Args>(args)...);
            auto& value = storage->value;
            storage_[index] = std::move(storage);

            return value;
        }

        template<typename T>
        const T& read() const noexcept
        {
            static const auto index = typeIndex<T>();
            pf_assert(index < storage_.size(), "Index out of bounds");

            return static_cast<Storage<T>*>(storage_[index].get())->value;
        }

        template<typename T>
        T& write() noexcept
        {
            static const auto index = typeIndex<T>();
            pf_assert(index < storage_.size(), "Index out of bounds");

            return static_cast<Storage<T>*>(storage_[index].get())->value;
        }

        template<typename T>
        [[nodiscard]] bool hasType() const noexcept
        {
            static const auto index = typeIndex<T>();

            return index < storage_.size() && storage_[index];
        }

        [[nodiscard]] bool hasType(std::uint32_t type_index) const noexcept
        {
            return type_index < storage_.size() && storage_[type_index];
        }

        TypeMap& operator=(const TypeMap&) = delete;

        TypeMap& operator=(TypeMap&& other) noexcept
        {
            if(std::addressof(other) == this) return *this;

            storage_ = std::move(other.storage_);

            return *this;
        }

    private:
        struct Erased
        {
            virtual ~Erased() noexcept = default;
        };

        template<typename T>
        struct Storage final : Erased
        {
            template<typename... Args>
            explicit Storage(Args&&... args) : value(std::forward<Args>(args)...) { }

            T value;
        };

        Vector<UniquePtr<Erased>> storage_;
    };
}
