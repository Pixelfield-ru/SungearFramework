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
        virtual ~IPhysicalDevice() = default;

        [[nodiscard]] virtual bool valid() const noexcept = 0;

        [[nodiscard]] core::String name() const noexcept
        {
            return name_;
        }

        [[nodiscard]] DeviceType type() const noexcept
        {
            return type_;
        }

    protected:
        core::String name_;
        DeviceType type_ = DeviceType::kIntegratedGpu;
    };
}
