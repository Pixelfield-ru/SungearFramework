//
// Created by stuka on 10.09.2026.
//

#pragma once

#include <string>

#include "types.h"
#include "sgf/core/memory.h"

namespace sgf::rhi
{
    struct IPhysicalDevice
    {
        using allocator_type = std::pmr::polymorphic_allocator<>;

        virtual ~IPhysicalDevice() = default;
        IPhysicalDevice() = default;
        IPhysicalDevice(const allocator_type& alloc) :
            name_(alloc)
        { }
        IPhysicalDevice(const IPhysicalDevice&) = default;
        IPhysicalDevice(const IPhysicalDevice& other, const allocator_type& alloc) :
            name_(other.name_, alloc),
            type_(other.type_)
        { }
        IPhysicalDevice(IPhysicalDevice&&) = default;
        IPhysicalDevice(IPhysicalDevice&& other, const allocator_type& alloc) noexcept :
            name_(std::move(other.name_), alloc),
            type_(other.type())
        { }

        [[nodiscard]] virtual bool valid() const noexcept = 0;

        [[nodiscard]] core::String name() const noexcept
        {
            return name_;
        }

        [[nodiscard]] DeviceType type() const noexcept
        {
            return type_;
        }

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return name_.get_allocator();
        }

        IPhysicalDevice& operator=(const IPhysicalDevice&) = default;
        IPhysicalDevice& operator=(IPhysicalDevice&&) noexcept = default;

    protected:
        core::String name_;
        DeviceType type_ = DeviceType::kIntegratedGpu;
    };
}
