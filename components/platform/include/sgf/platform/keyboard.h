//
// Created by stuka on 12.11.2026.
//

#pragma once

#include <array>
#include <bitset>
#include <cstdint>
#include <memory_resource>
#include <span>

#include "types.h"
#include "sgf/core/memory.h"

namespace sgf::platform
{
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
        friend struct Platform;

        using allocator_type = std::pmr::polymorphic_allocator<>;

        explicit Keyboard(const allocator_type& allocator = {}) :
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

        /// Clear last frame events.
        void clearFrame() noexcept;
        /// Clear held scancodes. GLFW is sending scancode 0 when focus was lost.
        void releaseScancodes() noexcept;
        /// Release all keyboard keys and scancodes.
        void releaseAll();
    };
}
