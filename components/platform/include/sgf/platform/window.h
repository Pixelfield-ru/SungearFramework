//
// Created by stuka on 13.10.2026.
//

#pragma once

#include <string_view>
#include <system_error>

#include "sgf/core/log.h"
#include "sgf/core/memory.h"

namespace sgf::platform
{
    enum class WindowClientApi
    {
        kNoApi,
        kOpenGL,
        kOpenGLES
    };

    struct WindowCreateInfo final
    {
        const core::LogCallback* debug_output {};

        WindowClientApi client_api = WindowClientApi::kNoApi;

        std::int32_t width {};
        std::int32_t height {};

        bool fullscreen {};

        std::string_view title;
    };

    struct Window final
    {
        using allocator_type = std::pmr::polymorphic_allocator<>;

        Window() noexcept;
        explicit Window(const allocator_type& alloc) noexcept;
        Window(const Window&) = delete;
        Window(Window&& other) noexcept;
        Window(Window&& other, const allocator_type& alloc) noexcept;
        ~Window() noexcept;

        void setTitle(std::string_view title) noexcept;

        void position(std::int32_t& x, std::int32_t& y) const noexcept;
        void setPosition(std::int32_t x, std::int32_t y) noexcept;

        void size(std::int32_t& width, std::int32_t& height) const noexcept;
        void setSize(std::int32_t width, std::int32_t height) noexcept;

        [[nodiscard]] allocator_type get_allocator() const noexcept;

        Window& operator=(const Window&) = delete;
        Window& operator=(Window&& other) noexcept;

        static std::error_code create(const WindowCreateInfo& create_info, Window& output) noexcept;

    private:
        struct Impl;

        std::pmr::memory_resource* resource_ = std::pmr::get_default_resource();
        core::UniquePtr<Impl> impl_;

        const core::LogCallback* debug_output_ {};

        void destroy() noexcept;
    };
}