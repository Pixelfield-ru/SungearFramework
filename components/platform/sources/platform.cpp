//
// Created by stuka on 13.10.2026.
//

#include "sgf/platform/platform.h"

#include <algorithm>

#include "GLFW/glfw3.h"
#include "sgf/core/errors.h"
#include "sgf/platform/window.h"
#include "sgf/platform/glfw/glfw_errors.h"
#include "sgf/platform/glfw/glfw_types.h"

static_assert(sgf::platform::kGamepadSlotCount == static_cast<std::size_t>(GLFW_JOYSTICK_LAST) + 1);
static_assert(static_cast<int>(sgf::platform::GamepadButton::kSouth) == GLFW_GAMEPAD_BUTTON_A);
static_assert(static_cast<int>(sgf::platform::GamepadButton::kDpadLeft) == GLFW_GAMEPAD_BUTTON_DPAD_LEFT);
static_assert(static_cast<int>(sgf::platform::GamepadButton::kCount) == GLFW_GAMEPAD_BUTTON_LAST + 1);
static_assert(static_cast<int>(sgf::platform::GamepadAxis::kLeftX) == GLFW_GAMEPAD_AXIS_LEFT_X);
static_assert(static_cast<int>(sgf::platform::GamepadAxis::kRightTrigger) == GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER);
static_assert(static_cast<int>(sgf::platform::GamepadAxis::kCount) == GLFW_GAMEPAD_AXIS_LAST + 1);

sgf::platform::Platform::~Platform() noexcept
{
    glfwTerminate();
}

std::error_code sgf::platform::Platform::init() noexcept
{
    if(initialized_) return {};

    glfwSetErrorCallback(glfwErrorCallback);

    if(!glfwInit())
    {
        core::LogCallback::error(debug_output_, "Cannot init GLFW library");
        return GLFWError::kCannotInitGLFW;
    }

    initialized_ = true;

    return {};
}

bool sgf::platform::Platform::initialized() const noexcept
{
    return initialized_;
}

std::uint32_t sgf::platform::Platform::openedWindowsCount() const noexcept
{
    return opened_windows_count_;
}

std::span<const sgf::platform::Gamepad> sgf::platform::Platform::gamepads() const noexcept
{
    return gamepads_;
}

std::error_code sgf::platform::Platform::updateGamepadMappings(
    std::string_view mappings, const std::pmr::polymorphic_allocator<>& alloc) const
{
    if(!initialized_) return core::Error::kUninitialized;

    const core::String text { mappings.begin(), mappings.end(), alloc };
    if(glfwUpdateGamepadMappings(text.c_str()) != GLFW_TRUE)
    {
        return GLFWError::kInvalidGamepadMappings;
    }

    return {};
}

sgf::platform::Window* sgf::platform::Platform::focusedWindow() noexcept
{
    return focused_window_;
}

void sgf::platform::Platform::setDebugOutput(const core::LogCallback& callback) noexcept
{
    debug_output_ = &callback;
}

void sgf::platform::Platform::poll() const noexcept
{
    for(auto* window : windows_)
    {
        window->keyboard().clearFrame();
        window->mouse().clearFrame();

        if(window->valid() && window->shouldClose())
            window->close();
    }

    glfwPollEvents();
    updateGamepads();
}

void sgf::platform::Platform::updateGamepads() const noexcept
{
    for(int jid = GLFW_JOYSTICK_1; jid <= GLFW_JOYSTICK_LAST; ++jid)
    {
        auto& pad = gamepads_[static_cast<std::size_t>(jid)];
        pad.clearFrame();

        if(glfwJoystickPresent(jid) != GLFW_TRUE)
        {
            pad.disconnect();
            continue;
        }

        const bool mapped = glfwJoystickIsGamepad(jid) == GLFW_TRUE;
        const char* name = mapped ? glfwGetGamepadName(jid) : nullptr;
        if(!name) name = glfwGetJoystickName(jid);

        GLFWgamepadstate state {};
        const bool have_state = mapped && glfwGetGamepadState(jid, &state) == GLFW_TRUE;
        pad.applyState(have_state,
                       std::span<const unsigned char> { state.buttons, sizeof(state.buttons) },
                       std::span<const float> { state.axes, sizeof(state.axes) / sizeof(float) },
                       name,
                       glfwGetJoystickGUID(jid));
    }
}

void sgf::platform::Platform::addWindow(Window* window) noexcept
{
    windows_.push_back(window);
}

void sgf::platform::Platform::removeWindow(Window* window) noexcept
{
    std::erase_if(windows_, [window](auto* wnd) {
        return wnd == window;
    });

    if(window == focused_window_)
    {
        defocusWindow();
    }
}

void sgf::platform::Platform::defocusWindow() noexcept
{
    focused_window_->keyboard_.releaseScancodes();
    focused_window_ = {};
}

void sgf::platform::Platform::glfwErrorCallback(int error, const char* description) noexcept
{
    core::LogCallback::print(platform().debug_output_, fromGlfwError(error), description ? description : "");
}

sgf::platform::Platform& sgf::platform::platform() noexcept
{
    static Platform instance;
    return instance;
}
