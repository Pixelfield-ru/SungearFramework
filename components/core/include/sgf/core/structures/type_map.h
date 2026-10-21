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
    // todo: use pmr
    struct TypeMap final
    {
        TypeMap(std::pmr::memory_resource* resource) noexcept :
            memory_resource_(resource),
            storage_(resource)
        {

        }

        TypeMap() noexcept = default;
        TypeMap(const TypeMap&) = default;
        TypeMap(TypeMap&&) = default;

        template<typename T, typename... Args>
        T& emplace(Args&&... args) noexcept
        {
            static const auto index = typeIndex<T>();
            if(index >= storage_.size())
            {
                storage_.resize(index + 1);
            }

            auto storage = core::make_unique<Storage<T>>(memory_resource_, std::forward<Args>(args)...);
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

        TypeMap& operator=(const TypeMap&) = default;
        TypeMap& operator=(TypeMap&&) = default;

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

        std::pmr::memory_resource* memory_resource_ = std::pmr::get_default_resource();
        Vector<UniquePtr<Erased>> storage_;
    };
}
