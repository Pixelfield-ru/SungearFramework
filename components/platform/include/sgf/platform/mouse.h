//
// Created by stuka on 12.11.2026.
//

#pragma once

#include <array>
#include <cstdint>
#include <memory_resource>
#include <span>

#include "types.h"
#include "sgf/core/memory.h"

namespace sgf::platform
{
    struct MouseButtonEvent
    {
        MouseButton button = MouseButton::kLeft;
        KeyAction action = KeyAction::kRelease;
        KeyMod mods = KeyMod::kNone;
    };

    struct Mouse final
    {
        friend struct Window;
        friend struct Platform;

        using allocator_type = std::pmr::polymorphic_allocator<>;

        explicit Mouse(const allocator_type& allocator = {}) :
            events_(allocator)
        { }
        Mouse(const Mouse&) = default;
        Mouse(const Mouse& other, const allocator_type& alloc) :
            buttons_(other.buttons_),
            events_(other.events_, alloc),
            x_(other.x_),
            y_(other.y_),
            delta_x_(other.delta_x_),
            delta_y_(other.delta_y_),
            scroll_x_(other.scroll_x_),
            scroll_y_(other.scroll_y_),
            inside_(other.inside_),
            has_position_(other.has_position_)
        { }
        Mouse(Mouse&& other) noexcept :
            buttons_(other.buttons_),
            events_(std::move(other.events_)),
            x_(other.x_),
            y_(other.y_),
            delta_x_(other.delta_x_),
            delta_y_(other.delta_y_),
            scroll_x_(other.scroll_x_),
            scroll_y_(other.scroll_y_),
            inside_(other.inside_),
            has_position_(other.has_position_)
        {
            other.clear();
        }
        Mouse(Mouse&& other, const allocator_type& alloc) noexcept :
            buttons_(other.buttons_),
            events_(std::move(other.events_), alloc),
            x_(other.x_),
            y_(other.y_),
            delta_x_(other.delta_x_),
            delta_y_(other.delta_y_),
            scroll_x_(other.scroll_x_),
            scroll_y_(other.scroll_y_),
            inside_(other.inside_),
            has_position_(other.has_position_)
        {
            other.clear();
        }

        [[nodiscard]] bool down(MouseButton button) const noexcept;
        [[nodiscard]] bool pressed(MouseButton button) const noexcept;
        [[nodiscard]] bool released(MouseButton button) const noexcept;

        [[nodiscard]] std::span<const MouseButtonEvent> events() const noexcept;

        void position(double& x, double& y) const noexcept;
        void delta(double& x, double& y) const noexcept;
        void scroll(double& x, double& y) const noexcept;

        [[nodiscard]] bool inside() const noexcept;

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return events_.get_allocator();
        }

        Mouse& operator=(const Mouse&) = default;
        Mouse& operator=(Mouse&& other) noexcept
        {
            if(this == &other) return *this;

            buttons_ = other.buttons_;
            events_ = std::move(other.events_);
            x_ = other.x_;
            y_ = other.y_;
            delta_x_ = other.delta_x_;
            delta_y_ = other.delta_y_;
            scroll_x_ = other.scroll_x_;
            scroll_y_ = other.scroll_y_;
            inside_ = other.inside_;
            has_position_ = other.has_position_;

            other.clear();

            return *this;
        }

    private:
        std::array<KeyFlags, static_cast<std::size_t>(MouseButton::kCount)> buttons_ {};
        core::Vector<MouseButtonEvent> events_;

        double x_ {};
        double y_ {};
        double delta_x_ {};
        double delta_y_ {};
        double scroll_x_ {};
        double scroll_y_ {};
        bool inside_ {};
        bool has_position_ {};

        [[nodiscard]] const KeyFlags* buttonFlags(MouseButton button) const noexcept;
        [[nodiscard]] KeyFlags* buttonFlags(MouseButton button) noexcept;

        void pushButton(MouseButton button, KeyAction action, KeyMod mods);
        void pushMove(double x, double y) noexcept;
        void pushScroll(double x, double y) noexcept;
        void setInside(bool inside) noexcept;

        /// Clear edges, the event queue, delta and scroll. Held buttons stay down.
        void clearFrame() noexcept;
        /// Release every held button.
        void releaseAll();
        void clear() noexcept;
    };
}
