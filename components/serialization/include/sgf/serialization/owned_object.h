//
// Created by stuka on 01.11.2026.
//

#pragma once

#include <memory_resource>
#include <utility>

#include "sgf/core/memory.h"

namespace sgf::serialization
{
    struct OwnedObject final
    {
        using deleter_t = void(std::pmr::memory_resource*, void*);

        OwnedObject() = default;
        OwnedObject(void* obj, std::pmr::memory_resource* resource, deleter_t deleter) noexcept :
            pointer_(obj),
            resource_(resource),
            deleter_(deleter)
        { }
        OwnedObject(const OwnedObject&) = delete;
        OwnedObject(OwnedObject&& other) noexcept :
            pointer_(std::exchange(other.pointer_, nullptr)),
            resource_(std::exchange(other.resource_, nullptr)),
            deleter_(std::exchange(other.deleter_, nullptr))
        { }

        ~OwnedObject()
        {
            if(pointer_)
            {
                deleter_(resource_, pointer_);
            }
        }

        template<typename T>
        core::UniquePtr<T> adopt() &&
        {
            core::MemoryResourceDeleter<T> deleter(resource_, static_cast<T*>(pointer_));
            deleter.destroy = deleter_;
            pointer_ = nullptr;

            return core::UniquePtr<T>(static_cast<T*>(deleter.object), deleter);
        }

        void* get() const noexcept
        {
            return pointer_;
        }

        void* release() noexcept
        {
            return std::exchange(pointer_, nullptr);
        }

        operator bool() const noexcept
        {
            return pointer_;
        }

        OwnedObject& operator=(const OwnedObject&) = delete;
        OwnedObject& operator=(OwnedObject&& other) noexcept
        {
            if(this == &other) return *this;

            if(pointer_)
            {
                deleter_(resource_, pointer_);
            }

            pointer_ = std::exchange(other.pointer_, nullptr);
            resource_ = std::exchange(other.resource_, nullptr);
            deleter_ = std::exchange(other.deleter_, nullptr);
            return *this;
        }

    private:
        void* pointer_ {};
        std::pmr::memory_resource* resource_ {};
        deleter_t* deleter_ {};
    };
}