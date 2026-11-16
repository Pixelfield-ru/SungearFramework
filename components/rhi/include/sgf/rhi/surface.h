//
// Created by stuka on 17.11.2026.
//

#pragma once

#include <system_error>

namespace sgf::platform
{
    struct Window;
}

namespace sgf::rhi
{
    struct IContext;

    struct SurfaceCreateInfo final
    {
        const IContext* context {};
        const platform::Window* window {};
    };

    struct ISurface
    {
        virtual ~ISurface() = default;

    private:
        virtual void destroy() = 0;
        [[nodiscard]] virtual std::error_code create(const SurfaceCreateInfo& create_info) = 0;
    };
}