//
// Created by stuka on 12.11.2026.
//

#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string_view>

#include "types.h"

namespace sgf::platform
{
    /// Standard layout. Button 0 of an unmapped joystick is not kSouth.
    enum class GamepadButton : std::uint8_t
    {
        kSouth = 0,
        kEast,
        kWest,
        kNorth,
        kLeftBumper,
        kRightBumper,
        kBack,
        kStart,
        kGuide,
        kLeftThumb,
        kRightThumb,
        kDpadUp,
        kDpadRight,
        kDpadDown,
        kDpadLeft,

        kCount
    };

    /// Raw axis values are -1..1. A trigger at rest is -1.
    enum class GamepadAxis : std::uint8_t
    {
        kLeftX = 0,
        kLeftY,
        kRightX,
        kRightY,
        kLeftTrigger,
        kRightTrigger,

        kCount
    };

    inline constexpr std::size_t kGamepadSlotCount = 16;

    struct Gamepad final
    {
        friend struct Platform;

        [[nodiscard]] bool connected() const noexcept;
        [[nodiscard]] bool mapped() const noexcept;

        [[nodiscard]] std::string_view name() const noexcept;
        [[nodiscard]] std::string_view guid() const noexcept;

        [[nodiscard]] bool down(GamepadButton button) const noexcept;
        [[nodiscard]] bool pressed(GamepadButton button) const noexcept;
        [[nodiscard]] bool released(GamepadButton button) const noexcept;

        [[nodiscard]] float axis(GamepadAxis axis) const noexcept;

    private:
        std::array<KeyFlags, static_cast<std::size_t>(GamepadButton::kCount)> buttons_ {};
        std::array<float, static_cast<std::size_t>(GamepadAxis::kCount)> axes_ {};
        std::array<char, 128> name_ {};
        std::array<char, 40> guid_ {};
        bool connected_ {};
        bool mapped_ {};

        [[nodiscard]] const KeyFlags* buttonFlags(GamepadButton button) const noexcept;
        [[nodiscard]] KeyFlags* buttonFlags(GamepadButton button) noexcept;

        /// Clear temporary frame data.
        void clearFrame() noexcept;
        /// Clear gamepad data.
        void disconnect() noexcept;
        void applyState(bool mapped, std::span<const unsigned char> buttons, std::span<const float> axes,
                        const char* name, const char* guid) noexcept;
        /// Release all keys with state equals to kDown.
        void releaseDown() noexcept;

        static void assignText(std::span<char> dst, const char* src) noexcept;
    };
}
