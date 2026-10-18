//
// Created by stuka on 13.10.2026.
//

#include "sgf/platform/glfw/glfw_window.h"

#include <memory>

#include "sgf/core/errors.h"
#include "sgf/platform/glfw/glfw_errors.h"
#include "sgf/platform/platform.h"

sgf::platform::GLFWWindow::GLFWWindow(GLFWWindow&& other) noexcept :
    handle_(std::exchange(other.handle_, {}))
{
}

sgf::platform::GLFWWindow::~GLFWWindow() noexcept
{
    destroy();
}

void sgf::platform::GLFWWindow::setTitle(std::string_view title) noexcept
{
    glfwSetWindowTitle(handle_, title.data());
}

void sgf::platform::GLFWWindow::position(std::int32_t& x, std::int32_t& y) const noexcept
{
    glfwGetWindowPos(handle_, &x, &y);
}

void sgf::platform::GLFWWindow::setPosition(std::int32_t x, std::int32_t y) noexcept
{
    glfwSetWindowPos(handle_, x, y);
}

void sgf::platform::GLFWWindow::size(std::int32_t& width, std::int32_t& height) const noexcept
{
    glfwGetWindowSize(handle_, &width, &height);
}

void sgf::platform::GLFWWindow::setSize(std::int32_t width, std::int32_t height) noexcept
{
    glfwSetWindowSize(handle_, width, height);
}

sgf::platform::GLFWWindow& sgf::platform::GLFWWindow::operator=(GLFWWindow&& other) noexcept
{
    if(this == std::addressof(other)) return *this;

    destroy();

    handle_ = std::exchange(other.handle_, {});

    return *this;
}

std::error_code sgf::platform::GLFWWindow::create(const WindowCreateInfo& create_info, GLFWWindow& output) noexcept
{
    output.debug_output_ = create_info.debug_output;

    if(!platform().initialized())
    {
        return core::Error::kUninitialized;
    }

    glfwDefaultWindowHints();

    switch(create_info.client_api)
    {
        case WindowClientApi::kNoApi:
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            break;
        case WindowClientApi::kOpenGL:
            glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
            break;
        case WindowClientApi::kOpenGLES:
            glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
            glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
            break;
    }

    output.handle_ = glfwCreateWindow(create_info.width, create_info.height, create_info.title.data(),
                                      create_info.fullscreen ? glfwGetPrimaryMonitor() : nullptr, nullptr);
    if (!output.handle_)
    {
        if(output.debug_output_)
        {
            (*output.debug_output_)(core::LogLevel::kError, "Cannot create GLFW window.");
        }
        return GLFWError::kCannotCreateWindow;
    }

    glfwSetWindowUserPointer(output.handle_, &output);
    glfwSetWindowCloseCallback(output.handle_, nativeCloseCallback);
    glfwSetWindowIconifyCallback(output.handle_, nativeIconifyCallback);
    glfwSetFramebufferSizeCallback(output.handle_, nativeFramebufferSizeCallback);

    if(create_info.client_api == WindowClientApi::kOpenGL)
    {
        glfwShowWindow(output.handle_);
    }

    glfwDefaultWindowHints();

    return {};
}

void sgf::platform::GLFWWindow::destroy() noexcept
{
    if(handle_)
    {
        glfwDestroyWindow(handle_);
        handle_ = {};
    }
}

void sgf::platform::GLFWWindow::nativeCloseCallback(GLFWwindow* window) noexcept
{
    auto* my_window = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
}

void sgf::platform::GLFWWindow::nativeIconifyCallback(GLFWwindow* window, int iconified) noexcept
{
    auto* my_window = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
}

void sgf::platform::GLFWWindow::nativeFramebufferSizeCallback(GLFWwindow* window, int width, int height) noexcept
{
    auto* my_window = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
}
