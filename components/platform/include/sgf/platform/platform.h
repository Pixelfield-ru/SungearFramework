//
// Created by stuka on 13.10.2026.
//

#pragma once

#include <array>
#include <span>
#include <string_view>
#include <system_error>

#include "sgf/core/log.h"
#include "sgf/core/memory.h"

#include "gamepad.h"

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

        [[nodiscard]] std::span<const Gamepad> gamepads() const noexcept;
        [[nodiscard]] std::error_code updateGamepadMappings(
            std::string_view mappings, const std::pmr::polymorphic_allocator<>& alloc = {}) const;

        Window* focusedWindow() noexcept;

        void setDebugOutput(const core::LogCallback& callback) noexcept;

    private:
        Platform() noexcept = default;

        const core::LogCallback* debug_output_ {};
        bool initialized_ {};

        core::Vector<Window*> windows_;
        Window* focused_window_ {};
        std::uint32_t opened_windows_count_ {};
        mutable std::array<Gamepad, kGamepadSlotCount> gamepads_ {};

        void addWindow(Window* window) noexcept;
        void removeWindow(Window* window) noexcept;
        void defocusWindow() noexcept;
        void updateGamepads() const noexcept;

        static void glfwErrorCallback(int error, const char* description) noexcept;
    };

    Platform& platform() noexcept;
}
