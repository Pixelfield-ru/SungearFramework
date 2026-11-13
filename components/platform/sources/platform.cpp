//
// Created by stuka on 13.10.2026.
//

#include "sgf/platform/platform.h"

#include <algorithm>

#include "GLFW/glfw3.h"
#include "sgf/platform/glfw/glfw_errors.h"

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
        if(debug_output_) (*debug_output_)(core::LogLevel::kError, "Cannot init GLFW library");
        return GLFWError::kCannotInitGLFW;
    }

    initialized_ = true;

    return {};
}

bool sgf::platform::Platform::initialized() const noexcept
{
    return initialized_;
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
        focused_window_ = {};
    }
}

void sgf::platform::Platform::glfwErrorCallback(int level, const char* description) noexcept
{
    auto* output = platform().debug_output_;
    if(output)
    {
        (*output)(core::LogLevel::kError, description);
    }
}

sgf::platform::Platform& sgf::platform::platform() noexcept
{
    static Platform instance;
    return instance;
}
