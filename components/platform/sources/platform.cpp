//
// Created by stuka on 13.10.2026.
//

#include "sgf/platform/platform.h"

#include <algorithm>

#include "GLFW/glfw3.h"
#include "sgf/platform/window.h"
#include "sgf/platform/glfw/glfw_errors.h"
#include "sgf/platform/glfw/glfw_types.h"

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

        if(window->valid() && window->shouldClose())
            window->close();
    }

    glfwPollEvents();
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
