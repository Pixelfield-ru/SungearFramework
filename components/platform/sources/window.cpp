//
// Created by stuka on 13.10.2026.
//

#include "sgf/platform/window.h"

#include <memory>
#include <GLFW/glfw3.h>

#include "sgf/core/errors.h"
#include "sgf/platform/glfw/glfw_errors.h"
#include "sgf/platform/platform.h"

struct sgf::platform::Window::Impl
{
    GLFWwindow* handle {};

    static void nativeCloseCallback(GLFWwindow* window) noexcept;
    static void nativeIconifyCallback(GLFWwindow* window, int iconified) noexcept;
    static void nativeFramebufferSizeCallback(GLFWwindow* window, int width, int height) noexcept;
};

sgf::platform::Window::Window() noexcept :
    impl_(core::makeUnique<Impl>(std::pmr::get_default_resource()))
{
}

sgf::platform::Window::Window(const allocator_type& alloc) noexcept :
    resource_(alloc.resource()),
    impl_(core::makeUnique<Impl>(alloc))
{

}

sgf::platform::Window::Window(Window&& other) noexcept :
    resource_(other.resource_),
    impl_(std::move(other.impl_)),
    debug_output_(other.debug_output_)
{
}

sgf::platform::Window::Window(Window&& other, const allocator_type& alloc) noexcept :
    resource_(other.resource_),
    impl_(std::move(other.impl_)),
    debug_output_(other.debug_output_)
{

}

sgf::platform::Window::~Window() noexcept
{
    destroy();
}

void sgf::platform::Window::setTitle(std::string_view title) noexcept
{
    glfwSetWindowTitle(impl_->handle, title.data());
}

void sgf::platform::Window::position(std::int32_t& x, std::int32_t& y) const noexcept
{
    glfwGetWindowPos(impl_->handle, &x, &y);
}

void sgf::platform::Window::setPosition(std::int32_t x, std::int32_t y) noexcept
{
    glfwSetWindowPos(impl_->handle, x, y);
}

void sgf::platform::Window::size(std::int32_t& width, std::int32_t& height) const noexcept
{
    glfwGetWindowSize(impl_->handle, &width, &height);
}

void sgf::platform::Window::setSize(std::int32_t width, std::int32_t height) noexcept
{
    glfwSetWindowSize(impl_->handle, width, height);
}

sgf::platform::Window::allocator_type sgf::platform::Window::get_allocator() const noexcept
{
    return resource_;
}

sgf::platform::Window& sgf::platform::Window::operator=(Window&& other) noexcept
{
    if(this == std::addressof(other)) return *this;

    destroy();

    resource_ = other.resource_;
    impl_ = std::move(other.impl_);
    debug_output_ = other.debug_output_;

    return *this;
}

std::error_code sgf::platform::Window::create(const WindowCreateInfo& create_info, Window& output) noexcept
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

    const auto& impl = output.impl_;

    impl->handle = glfwCreateWindow(create_info.width, create_info.height, create_info.title.data(),
                                            create_info.fullscreen ? glfwGetPrimaryMonitor() : nullptr, nullptr);
    if (!impl->handle)
    {
        if(output.debug_output_)
        {
            (*output.debug_output_)(core::LogLevel::kError, "Cannot create GLFW window.");
        }
        return GLFWError::kCannotCreateWindow;
    }

    glfwSetWindowUserPointer(impl->handle, &output);
    glfwSetWindowCloseCallback(impl->handle, &Impl::nativeCloseCallback);
    glfwSetWindowIconifyCallback(impl->handle, &Impl::nativeIconifyCallback);
    glfwSetFramebufferSizeCallback(impl->handle, &Impl::nativeFramebufferSizeCallback);

    if(create_info.client_api == WindowClientApi::kOpenGL)
    {
        glfwShowWindow(impl->handle);
    }

    glfwDefaultWindowHints();

    return {};
}

void sgf::platform::Window::destroy() noexcept
{
    if(impl_->handle)
    {
        glfwDestroyWindow(impl_->handle);
        impl_->handle = {};
    }
}

void sgf::platform::Window::Impl::nativeCloseCallback(GLFWwindow* window) noexcept
{
    auto* my_window = static_cast<Window*>(glfwGetWindowUserPointer(window));
}

void sgf::platform::Window::Impl::nativeIconifyCallback(GLFWwindow* window, int iconified) noexcept
{
    auto* my_window = static_cast<Window*>(glfwGetWindowUserPointer(window));
}

void sgf::platform::Window::Impl::nativeFramebufferSizeCallback(GLFWwindow* window, int width, int height) noexcept
{
    auto* my_window = static_cast<Window*>(glfwGetWindowUserPointer(window));
}
