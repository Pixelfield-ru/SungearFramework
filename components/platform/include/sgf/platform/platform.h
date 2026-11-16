//
// Created by stuka on 13.10.2026.
//

#pragma once

#include <system_error>

#include "sgf/core/log.h"
#include "sgf/core/memory.h"

namespace sgf::platform
{
    struct Window;

    struct Platform final
    {
        friend struct Window;

        friend Platform& platform() noexcept;

        ~Platform() noexcept;
        Platform(const Platform&) = delete;
        Platform(Platform&&) = delete;

        void poll() const noexcept;

        [[nodiscard]] std::error_code init() noexcept;
        [[nodiscard]] bool initialized() const noexcept;

        [[nodiscard]] std::uint32_t openedWindowsCount() const noexcept;

        Window* focusedWindow() noexcept;

        void setDebugOutput(const core::LogCallback& callback) noexcept;

    private:
        Platform() noexcept = default;

        const core::LogCallback* debug_output_ {};
        bool initialized_ {};

        core::Vector<Window*> windows_;
        Window* focused_window_ {};
        std::uint32_t opened_windows_count_ {};

        void addWindow(Window* window) noexcept;
        void removeWindow(Window* window) noexcept;
        void defocusWindow() noexcept;

        static void glfwErrorCallback(int error, const char* description) noexcept;
    };

    Platform& platform() noexcept;
}
