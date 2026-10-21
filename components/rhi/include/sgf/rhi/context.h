//
// Created by stuka on 10.09.2026.
//

#pragma once

#include <expected>
#include <string>
#include <functional>

#include "sgf/core/log.h"
#include "logical_device.h"

namespace sgf::rhi
{
    struct ContextCreateInfo
    {
        /// Required.
        core::String app_name;
        /// Required.
        core::String engine_name;
        /// Optional.
        bool enable_api_debug_output {};
        //// Required.
        GLFWwindow* window {};
        /// Optional.
        const core::LogCallback* rhi_debug_output {};

        std::pmr::memory_resource* memory_resource = std::pmr::get_default_resource();
    };

    struct IPhysicalDevice;

    struct IContext
    {
        virtual ~IContext() = default;

        [[nodiscard]] virtual bool valid() const noexcept = 0;

        [[nodiscard]] virtual core::Vector<core::SharedPtr<IPhysicalDevice>> physicalDevices() noexcept = 0;

        [[nodiscard]] virtual std::expected<core::UniquePtr<ILogicalDevice>, std::error_code> createLogicalDevice(const LogicalDeviceCreateInfo& create_info) noexcept = 0;

        void printDebugMessage(core::LogLevel level, std::string_view msg) const noexcept
        {
            if(rhi_debug_output_)
            {
                (*rhi_debug_output_)(level, msg);
            }
        }

    protected:
        const core::LogCallback* rhi_debug_output_ {};
    };
}
