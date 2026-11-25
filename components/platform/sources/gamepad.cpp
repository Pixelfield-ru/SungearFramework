//
// Created by stuka on 25.11.2026.
//

#include "sgf/platform/gamepad.h"

#include <algorithm>
#include <cstring>
#include <utility>

namespace
{
    bool hasFlag(sgf::platform::KeyFlags flags, sgf::platform::KeyFlags flag) noexcept
    {
        return (flags & flag) == flag;
    }
}

const sgf::platform::KeyFlags* sgf::platform::Gamepad::buttonFlags(GamepadButton button) const noexcept
{
    const auto index = static_cast<std::size_t>(std::to_underlying(button));
    if(index >= buttons_.size()) return nullptr;

    return &buttons_[index];
}

sgf::platform::KeyFlags* sgf::platform::Gamepad::buttonFlags(GamepadButton button) noexcept
{
    return const_cast<KeyFlags*>(std::as_const(*this).buttonFlags(button));
}

bool sgf::platform::Gamepad::connected() const noexcept
{
    return connected_;
}

bool sgf::platform::Gamepad::mapped() const noexcept
{
    return mapped_;
}

std::string_view sgf::platform::Gamepad::name() const noexcept
{
    return name_.data();
}

std::string_view sgf::platform::Gamepad::guid() const noexcept
{
    return guid_.data();
}

bool sgf::platform::Gamepad::down(GamepadButton button) const noexcept
{
    const KeyFlags* flags = buttonFlags(button);
    return flags && hasFlag(*flags, KeyFlags::kDown);
}

bool sgf::platform::Gamepad::pressed(GamepadButton button) const noexcept
{
    const KeyFlags* flags = buttonFlags(button);
    return flags && hasFlag(*flags, KeyFlags::kPressed);
}

bool sgf::platform::Gamepad::released(GamepadButton button) const noexcept
{
    const KeyFlags* flags = buttonFlags(button);
    return flags && hasFlag(*flags, KeyFlags::kReleased);
}

float sgf::platform::Gamepad::axis(GamepadAxis axis) const noexcept
{
    const auto index = static_cast<std::size_t>(std::to_underlying(axis));
    if(index >= axes_.size()) return 0.f;

    return axes_[index];
}

void sgf::platform::Gamepad::clearFrame() noexcept
{
    for(auto& flags : buttons_)
    {
        flags &= ~(KeyFlags::kReleased | KeyFlags::kPressed);
    }
}

void sgf::platform::Gamepad::disconnect() noexcept
{
    if(!connected_) return;

    releaseDown();
    connected_ = false;
    mapped_ = false;
    name_.fill('\0');
    guid_.fill('\0');
    axes_.fill(0.f);
}

void sgf::platform::Gamepad::applyState(bool mapped, std::span<const unsigned char> buttons,
                                        std::span<const float> axes,
                                        const char* name, const char* guid) noexcept
{
    connected_ = true;
    assignText(name_, name);
    assignText(guid_, guid);

    // unknown model so buttons are unknown
    if(!mapped)
    {
        releaseDown();
        mapped_ = false;
        axes_.fill(0.f);
        return;
    }

    mapped_ = true;

    // applying buttons states
    for(std::size_t index = 0; index < buttons_.size(); ++index)
    {
        const bool down = index < buttons.size() && buttons[index] != 0;
        auto& flags = buttons_[index];
        const bool was_down = hasFlag(flags, KeyFlags::kDown);

        if(down && !was_down)
        {
            flags |= KeyFlags::kDown | KeyFlags::kPressed;
        }
        else if(!down && was_down)
        {
            flags &= ~KeyFlags::kDown;
            flags |= KeyFlags::kReleased;
        }
    }

    // applying axes states
    for(std::size_t index = 0; index < axes_.size(); ++index)
    {
        axes_[index] = index < axes.size() ? axes[index] : 0.f;
    }
}

void sgf::platform::Gamepad::releaseDown() noexcept
{
    for(auto& flags : buttons_)
    {
        if(!hasFlag(flags, KeyFlags::kDown)) continue;

        flags &= ~KeyFlags::kDown;
        flags |= KeyFlags::kReleased;
    }
}

void sgf::platform::Gamepad::assignText(std::span<char> dst, const char* src) noexcept
{
    if(dst.empty()) return;

    if(!src)
    {
        dst[0] = '\0';
        return;
    }

    const std::size_t count = std::min(std::strlen(src), dst.size() - 1);
    std::memcpy(dst.data(), src, count);
    dst[count] = '\0';
}
