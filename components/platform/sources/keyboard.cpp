//
// Created by stuka on 14.11.2026.
//

#include "sgf/platform/keyboard.h"

#include <utility>

namespace
{
    bool hasFlag(sgf::platform::KeyFlags flags, sgf::platform::KeyFlags flag) noexcept
    {
        return (flags & flag) == flag;
    }

    void applyAction(sgf::platform::KeyFlags& flags, sgf::platform::KeyAction action) noexcept
    {
        if(action == sgf::platform::KeyAction::kPress)
        {
            flags |= sgf::platform::KeyFlags::kDown | sgf::platform::KeyFlags::kPressed;
        }
        else if(action == sgf::platform::KeyAction::kRelease)
        {
            flags &= ~sgf::platform::KeyFlags::kDown;
            flags |= sgf::platform::KeyFlags::kReleased;
        }
    }
}

const sgf::platform::KeyFlags* sgf::platform::Keyboard::keyFlags(KeyboardKey key) const noexcept
{
    if(key == KeyboardKey::kUnknown) return nullptr;

    const auto index = static_cast<std::size_t>(std::to_underlying(key));
    if(index >= keys_.size()) return nullptr;

    return &keys_[index];
}

sgf::platform::KeyFlags* sgf::platform::Keyboard::keyFlags(KeyboardKey key) noexcept
{
    return const_cast<KeyFlags*>(std::as_const(*this).keyFlags(key));
}

const sgf::platform::KeyFlags* sgf::platform::Keyboard::scancodeFlags(std::int32_t scancode) const noexcept
{
    if(scancode < 0) return nullptr;

    const auto index = static_cast<std::size_t>(scancode);
    if(index >= scancodes_.size()) return nullptr;

    return &scancodes_[index];
}

sgf::platform::KeyFlags* sgf::platform::Keyboard::scancodeFlags(std::int32_t scancode) noexcept
{
    return const_cast<KeyFlags*>(std::as_const(*this).scancodeFlags(scancode));
}

bool sgf::platform::Keyboard::down(KeyboardKey key) const noexcept
{
    const KeyFlags* flags = keyFlags(key);
    return flags && hasFlag(*flags, KeyFlags::kDown);
}

bool sgf::platform::Keyboard::pressed(KeyboardKey key) const noexcept
{
    const KeyFlags* flags = keyFlags(key);
    return flags && hasFlag(*flags, KeyFlags::kPressed);
}

bool sgf::platform::Keyboard::released(KeyboardKey key) const noexcept
{
    const KeyFlags* flags = keyFlags(key);
    return flags && hasFlag(*flags, KeyFlags::kReleased);
}

bool sgf::platform::Keyboard::down(std::int32_t scancode) const noexcept
{
    const KeyFlags* flags = scancodeFlags(scancode);
    return flags && hasFlag(*flags, KeyFlags::kDown);
}

bool sgf::platform::Keyboard::pressed(std::int32_t scancode) const noexcept
{
    const KeyFlags* flags = scancodeFlags(scancode);
    return flags && hasFlag(*flags, KeyFlags::kPressed);
}

bool sgf::platform::Keyboard::released(std::int32_t scancode) const noexcept
{
    const KeyFlags* flags = scancodeFlags(scancode);
    return flags && hasFlag(*flags, KeyFlags::kReleased);
}

std::span<const sgf::platform::KeyEvent> sgf::platform::Keyboard::events() const noexcept
{
    return events_;
}

std::span<const char32_t> sgf::platform::Keyboard::text() const noexcept
{
    return text_;
}

void sgf::platform::Keyboard::pushKey(KeyboardKey key, std::int32_t scancode, KeyAction action, KeyMod mods)
{
    if(KeyFlags* flags = keyFlags(key))
    {
        applyAction(*flags, action);
    }

    if(KeyFlags* flags = scancodeFlags(scancode))
    {
        applyAction(*flags, action);
    }

    events_.push_back(KeyEvent { key, scancode, action, mods });
}

void sgf::platform::Keyboard::pushText(char32_t codepoint)
{
    text_.push_back(codepoint);
}

void sgf::platform::Keyboard::clearFrame() noexcept
{
    for(auto& flag : keys_)
    {
        // clear temporary flags
        flag &= ~(KeyFlags::kReleased | KeyFlags::kPressed);
    }

    for(auto& flag : scancodes_)
    {
        // clear temporary flags
        flag &= ~(KeyFlags::kReleased | KeyFlags::kPressed);
    }

    events_.clear();
    text_.clear();
}

void sgf::platform::Keyboard::releaseAll()
{
    for(std::size_t index = 0; index < keys_.size(); ++index)
    {
        auto& flags = keys_[index];
        if(!hasFlag(flags, KeyFlags::kDown)) continue;

        flags &= ~KeyFlags::kDown;
        flags |= KeyFlags::kReleased;

        // add event that the key was released
        events_.push_back(KeyEvent {
            static_cast<KeyboardKey>(index),
            -1,
            KeyAction::kRelease,
            KeyMod::kNone
        });
    }

    for(std::size_t index = 0; index < scancodes_.size(); ++index)
    {
        auto& flags = scancodes_[index];
        if(!hasFlag(flags, KeyFlags::kDown)) continue;

        flags &= ~KeyFlags::kDown;
        flags |= KeyFlags::kReleased;

        // add event that the scancode was released
        events_.push_back(KeyEvent {
            KeyboardKey::kUnknown,
            static_cast<std::int32_t>(index),
            KeyAction::kRelease,
            KeyMod::kNone
        });
    }
}
