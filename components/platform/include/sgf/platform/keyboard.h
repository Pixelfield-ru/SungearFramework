//
// Created by stuka on 12.11.2026.
//

#pragma once

#include <array>
#include <bitset>
#include <cstdint>
#include <memory_resource>
#include <span>

#include "sgf/core/memory.h"

namespace sgf::platform
{
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

    struct KeyEvent
    {
        KeyboardKey key = KeyboardKey::kUnknown;
        std::int32_t scancode {};
        KeyAction action = KeyAction::kRelease;
        KeyMod mods = KeyMod::kNone;
    };

    struct Keyboard final
    {
        friend struct Window;

        using allocator_type = std::pmr::polymorphic_allocator<>;

        Keyboard() = default;
        explicit Keyboard(const allocator_type& allocator) :
            events_(allocator),
            text_(allocator)
        { }
        Keyboard(const Keyboard&) = default;
        Keyboard(const Keyboard& other, const allocator_type& alloc) :
            keys_(other.keys_),
            scancodes_(other.scancodes_),
            events_(other.events_, alloc),
            text_(other.text_, alloc)
        { }
        Keyboard(Keyboard&& other) noexcept :
            keys_(other.keys_),
            scancodes_(other.scancodes_),
            events_(std::move(other.events_)),
            text_(std::move(other.text_))
        {
            other.keys_.fill(KeyFlags::kNone);
            other.scancodes_.fill(KeyFlags::kNone);
        }
        Keyboard(Keyboard&& other, const allocator_type& alloc) noexcept :
            keys_(other.keys_),
            scancodes_(other.scancodes_),
            events_(std::move(other.events_), alloc),
            text_(std::move(other.text_), alloc)
        {
            other.keys_.fill(KeyFlags::kNone);
            other.scancodes_.fill(KeyFlags::kNone);
        }

        [[nodiscard]] bool down(KeyboardKey key) const noexcept;
        [[nodiscard]] bool pressed(KeyboardKey key) const noexcept;
        [[nodiscard]] bool released(KeyboardKey key) const noexcept;

        [[nodiscard]] bool down(std::int32_t scancode) const noexcept;
        [[nodiscard]] bool pressed(std::int32_t scancode) const noexcept;
        [[nodiscard]] bool released(std::int32_t scancode) const noexcept;

        [[nodiscard]] std::span<const KeyEvent> events() const noexcept;
        [[nodiscard]] std::span<const char32_t> text() const noexcept;

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return events_.get_allocator();
        }

        Keyboard& operator=(const Keyboard&) = default;
        Keyboard& operator=(Keyboard&& other) noexcept
        {
            if(this == &other) return *this;

            keys_ = other.keys_;
            scancodes_ = other.scancodes_;
            events_ = std::move(other.events_);
            text_ = std::move(other.text_);
            other.keys_.fill(KeyFlags::kNone);
            other.scancodes_.fill(KeyFlags::kNone);

            return *this;
        }

    private:
        std::array<KeyFlags, static_cast<std::size_t>(KeyboardKey::kCount)> keys_ {};
        std::array<KeyFlags, 1024> scancodes_ {};

        core::Vector<KeyEvent> events_;
        core::Vector<char32_t> text_;

        [[nodiscard]] const KeyFlags* keyFlags(KeyboardKey key) const noexcept;
        [[nodiscard]] KeyFlags* keyFlags(KeyboardKey key) noexcept;
        [[nodiscard]] const KeyFlags* scancodeFlags(std::int32_t scancode) const noexcept;
        [[nodiscard]] KeyFlags* scancodeFlags(std::int32_t scancode) noexcept;

        void pushKey(KeyboardKey key, std::int32_t scancode, KeyAction action, KeyMod mods);
        void pushText(char32_t codepoint);

        void clearFrame() noexcept;
        void releaseAll();
    };
}
