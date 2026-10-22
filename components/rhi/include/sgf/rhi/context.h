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
        using allocator_type = std::pmr::polymorphic_allocator<>;

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

        ContextCreateInfo() = default;
        ContextCreateInfo(const allocator_type& alloc) :
            app_name(alloc),
            engine_name(alloc)
        { }
        ContextCreateInfo(const ContextCreateInfo&) = default;
        ContextCreateInfo(const ContextCreateInfo& other, const allocator_type& alloc) :
            app_name(other.app_name, alloc),
            engine_name(other.engine_name, alloc),
            enable_api_debug_output(other.enable_api_debug_output),
            window(other.window),
            rhi_debug_output(other.rhi_debug_output)
        { }
        ContextCreateInfo(ContextCreateInfo&&) = default;
        ContextCreateInfo(ContextCreateInfo&& other, const allocator_type& alloc) :
            app_name(std::move(other.app_name), alloc),
            engine_name(std::move(other.engine_name), alloc),
            enable_api_debug_output(other.enable_api_debug_output),
            window(other.window),
            rhi_debug_output(other.rhi_debug_output)
        { }

        virtual ~ContextCreateInfo() = default;

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return app_name.get_allocator();
        }

        ContextCreateInfo& operator=(const ContextCreateInfo&) = default;
        ContextCreateInfo& operator=(ContextCreateInfo&&) noexcept = default;
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
