//
// Created by stuka on 13.10.2026.
//

#pragma once

#include <system_error>
#include <span>
#include <chrono>

#include "sgf/core/structures/type_map.h"
#include "sgf/runtime/execution/domain.h"

#include "system_schedule.h"

namespace sgf::runtime
{
    struct SystemContext final
    {
        using allocator_type = std::pmr::polymorphic_allocator<>;

        core::TypeMap resources;

        SystemContext(const allocator_type& alloc) noexcept :
            resources(alloc)
        { }
        SystemContext() noexcept = default;
        SystemContext(const SystemContext& other) noexcept = delete;
        SystemContext(SystemContext&&) noexcept = default;
        SystemContext(SystemContext&& other, const allocator_type& alloc) :
            resources(std::move(other.resources), alloc)
        { }

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return resources.get_allocator();
        }

        SystemContext& operator=(const SystemContext&) = delete;
        SystemContext& operator=(SystemContext&&) noexcept = default;
    };

    struct ISystem
    {
        virtual ~ISystem() = default;

        /**
         * @return Indices of types that the system reads.
         */
        [[nodiscard]] virtual std::span<const std::uint32_t> reads() const noexcept = 0;
        /**
        * @return Indices of types that the system writes.
        */
        [[nodiscard]] virtual std::span<const std::uint32_t> writes() const noexcept = 0;

        /**
        * Execute system once.
        * @param context System context.
        * @return Result code.
        */
        virtual std::error_code execute(SystemContext& context) = 0;

        /**
        * Initialize system.
        * @param context System context.
        * @return Result code.
        */
        virtual std::error_code initialize(SystemContext& context) = 0;

        [[nodiscard]] ExecutionDomain executionDomain() const noexcept
        {
            return execution_domain_;
        }

        [[nodiscard]] SystemScheduleType schedule() const noexcept
        {
            return schedule_;
        }

    protected:
        ExecutionDomain execution_domain_ = ExecutionDomain::kAny;
        SystemScheduleType schedule_ = SystemScheduleType::kFrame;
    };
}
