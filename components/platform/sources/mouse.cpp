//
// Created by stuka on 25.11.2026.
//

#include "sgf/platform/mouse.h"

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

const sgf::platform::KeyFlags* sgf::platform::Mouse::buttonFlags(MouseButton button) const noexcept
{
    const auto index = static_cast<std::size_t>(std::to_underlying(button));
    if(index >= buttons_.size()) return nullptr;

    return &buttons_[index];
}

sgf::platform::KeyFlags* sgf::platform::Mouse::buttonFlags(MouseButton button) noexcept
{
    return const_cast<KeyFlags*>(std::as_const(*this).buttonFlags(button));
}

bool sgf::platform::Mouse::down(MouseButton button) const noexcept
{
    const KeyFlags* flags = buttonFlags(button);
    return flags && hasFlag(*flags, KeyFlags::kDown);
}

bool sgf::platform::Mouse::pressed(MouseButton button) const noexcept
{
    const KeyFlags* flags = buttonFlags(button);
    return flags && hasFlag(*flags, KeyFlags::kPressed);
}

bool sgf::platform::Mouse::released(MouseButton button) const noexcept
{
    const KeyFlags* flags = buttonFlags(button);
    return flags && hasFlag(*flags, KeyFlags::kReleased);
}

std::span<const sgf::platform::MouseButtonEvent> sgf::platform::Mouse::events() const noexcept
{
    return events_;
}

void sgf::platform::Mouse::position(double& x, double& y) const noexcept
{
    x = x_;
    y = y_;
}

void sgf::platform::Mouse::delta(double& x, double& y) const noexcept
{
    x = delta_x_;
    y = delta_y_;
}

void sgf::platform::Mouse::scroll(double& x, double& y) const noexcept
{
    x = scroll_x_;
    y = scroll_y_;
}

bool sgf::platform::Mouse::inside() const noexcept
{
    return inside_;
}

void sgf::platform::Mouse::pushButton(MouseButton button, KeyAction action, KeyMod mods)
{
    if(KeyFlags* flags = buttonFlags(button))
    {
        applyAction(*flags, action);
    }

    events_.push_back(MouseButtonEvent { button, action, mods });
}

void sgf::platform::Mouse::pushMove(double x, double y) noexcept
{
    if(has_position_)
    {
        delta_x_ += x - x_;
        delta_y_ += y - y_;
    }

    x_ = x;
    y_ = y;
    has_position_ = true;
}

void sgf::platform::Mouse::pushScroll(double x, double y) noexcept
{
    scroll_x_ += x;
    scroll_y_ += y;
}

void sgf::platform::Mouse::setInside(bool inside) noexcept
{
    inside_ = inside;
}

void sgf::platform::Mouse::clearFrame() noexcept
{
    for(auto& flags : buttons_)
    {
        flags &= ~(KeyFlags::kReleased | KeyFlags::kPressed);
    }

    events_.clear();
    delta_x_ = {};
    delta_y_ = {};
    scroll_x_ = {};
    scroll_y_ = {};
}

void sgf::platform::Mouse::releaseAll()
{
    for(std::size_t index = 0; index < buttons_.size(); ++index)
    {
        auto& flags = buttons_[index];
        if(!hasFlag(flags, KeyFlags::kDown)) continue;

        flags &= ~KeyFlags::kDown;
        flags |= KeyFlags::kReleased;

        events_.push_back(MouseButtonEvent {
            static_cast<MouseButton>(index),
            KeyAction::kRelease,
            KeyMod::kNone
        });
    }
}

void sgf::platform::Mouse::clear() noexcept
{
    buttons_.fill(KeyFlags::kNone);
    events_.clear();
    x_ = {};
    y_ = {};
    delta_x_ = {};
    delta_y_ = {};
    scroll_x_ = {};
    scroll_y_ = {};
    inside_ = {};
    has_position_ = {};
}
