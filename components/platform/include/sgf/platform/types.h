//
// Created by stuka on 16.11.2026.
//

#pragma once

#include <cstdint>

namespace sgf::platform
{
    /// Dense codes for a state array. Backend button numbers are mapped into these.
    enum class MouseButton : std::uint8_t
    {
        kLeft = 0,
        kRight,
        kMiddle,
        kButton4,
        kButton5,
        kButton6,
        kButton7,
        kButton8,

        kCount
    };

    /// Dense codes for a state array. Backend key numbers are mapped into these.
    enum class KeyboardKey : std::uint8_t
    {
        kUnknown = 0,

        kSpace,
        kApostrophe,
        kComma,
        kMinus,
        kPeriod,
        kSlash,
        k0,
        k1,
        k2,
        k3,
        k4,
        k5,
        k6,
        k7,
        k8,
        k9,
        kSemicolon,
        kEqual,
        kA,
        kB,
        kC,
        kD,
        kE,
        kF,
        kG,
        kH,
        kI,
        kJ,
        kK,
        kL,
        kM,
        kN,
        kO,
        kP,
        kQ,
        kR,
        kS,
        kT,
        kU,
        kV,
        kW,
        kX,
        kY,
        kZ,
        kLeftBracket,
        kBackslash,
        kRightBracket,
        kGraveAccent,
        kWorld1,
        kWorld2,

        kEscape,
        kEnter,
        kTab,
        kBackspace,
        kInsert,
        kDelete,
        kRight,
        kLeft,
        kDown,
        kUp,
        kPageUp,
        kPageDown,
        kHome,
        kEnd,
        kCapsLock,
        kScrollLock,
        kNumLock,
        kPrintScreen,
        kPause,
        kF1,
        kF2,
        kF3,
        kF4,
        kF5,
        kF6,
        kF7,
        kF8,
        kF9,
        kF10,
        kF11,
        kF12,
        kF13,
        kF14,
        kF15,
        kF16,
        kF17,
        kF18,
        kF19,
        kF20,
        kF21,
        kF22,
        kF23,
        kF24,
        kF25,
        kKeypad0,
        kKeypad1,
        kKeypad2,
        kKeypad3,
        kKeypad4,
        kKeypad5,
        kKeypad6,
        kKeypad7,
        kKeypad8,
        kKeypad9,
        kKeypadDecimal,
        kKeypadDivide,
        kKeypadMultiply,
        kKeypadSubtract,
        kKeypadAdd,
        kKeypadEnter,
        kKeypadEqual,
        kLeftShift,
        kLeftControl,
        kLeftAlt,
        kLeftSuper,
        kRightShift,
        kRightControl,
        kRightAlt,
        kRightSuper,
        kMenu,

        kCount
    };

    enum class KeyAction : std::uint8_t
    {
        kRelease,
        kPress,
        kRepeat
    };

    enum class KeyFlags : std::uint8_t
    {
        kNone     = 0,
        kDown     = 1 << 0,
        kPressed  = 1 << 1,
        kReleased = 1 << 2
    };

    constexpr KeyFlags operator|(KeyFlags a, KeyFlags b) noexcept
    {
        return static_cast<KeyFlags>(static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b));
    }

    constexpr KeyFlags operator&(KeyFlags a, KeyFlags b) noexcept
    {
        return static_cast<KeyFlags>(static_cast<std::uint8_t>(a) & static_cast<std::uint8_t>(b));
    }

    constexpr KeyFlags operator~(KeyFlags value) noexcept
    {
        return static_cast<KeyFlags>(static_cast<std::uint8_t>(~static_cast<std::uint8_t>(value)));
    }

    constexpr KeyFlags& operator|=(KeyFlags& a, KeyFlags b) noexcept
    {
        a = a | b;
        return a;
    }

    constexpr KeyFlags& operator&=(KeyFlags& a, KeyFlags b) noexcept
    {
        a = a & b;
        return a;
    }

    enum class KeyMod : std::uint8_t
    {
        kNone     = 0,
        kShift    = 1 << 0,
        kControl  = 1 << 1,
        kAlt      = 1 << 2,
        kSuper    = 1 << 3,
        kCapsLock = 1 << 4,
        kNumLock  = 1 << 5
    };

    constexpr KeyMod operator|(KeyMod a, KeyMod b) noexcept
    {
        return static_cast<KeyMod>(static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b));
    }

    constexpr KeyMod operator&(KeyMod a, KeyMod b) noexcept
    {
        return static_cast<KeyMod>(static_cast<std::uint8_t>(a) & static_cast<std::uint8_t>(b));
    }

    constexpr KeyMod operator~(KeyMod value) noexcept
    {
        return static_cast<KeyMod>(static_cast<std::uint8_t>(~static_cast<std::uint8_t>(value)));
    }

    constexpr KeyMod& operator|=(KeyMod& a, KeyMod b) noexcept
    {
        a = a | b;
        return a;
    }

    constexpr KeyMod& operator&=(KeyMod& a, KeyMod b) noexcept
    {
        a = a & b;
        return a;
    }
}