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

    static void closeCallback(GLFWwindow* window) noexcept;
    static void iconifyCallback(GLFWwindow* window, int iconified) noexcept;
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height) noexcept;
    static void windowFocusedCallback(GLFWwindow* window, int focused) noexcept;
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) noexcept;
};

sgf::platform::Window::Window() noexcept :
    impl_(core::makeUnique<Impl>(std::pmr::get_default_resource()))
{
    platform().addWindow(this);
}

sgf::platform::Window::Window(const allocator_type& alloc) noexcept :
    resource_(alloc.resource()),
    impl_(core::makeUnique<Impl>(alloc))
{
    platform().addWindow(this);
}

sgf::platform::Window::Window(Window&& other) noexcept :
    resource_(other.resource_),
    impl_(std::move(other.impl_)),
    debug_output_(other.debug_output_)
{
    platform().addWindow(this);
    adoptNativeOwner(other);
}

sgf::platform::Window::Window(Window&& other, const allocator_type& alloc) noexcept :
    resource_(other.resource_),
    impl_(std::move(other.impl_)),
    debug_output_(other.debug_output_)
{
    platform().addWindow(this);
    adoptNativeOwner(other);
}

sgf::platform::Window::~Window() noexcept
{
    destroy();
    platform().removeWindow(this);
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
    setFocused(false);

    resource_ = other.resource_;
    impl_ = std::move(other.impl_);
    debug_output_ = other.debug_output_;

    adoptNativeOwner(other);

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
    glfwSetWindowCloseCallback(impl->handle, &Impl::closeCallback);
    glfwSetWindowIconifyCallback(impl->handle, &Impl::iconifyCallback);
    glfwSetFramebufferSizeCallback(impl->handle, &Impl::framebufferSizeCallback);
    glfwSetWindowFocusCallback(impl->handle, &Impl::windowFocusedCallback);
    glfwSetKeyCallback(impl->handle, &Impl::keyCallback);

    if(create_info.client_api == WindowClientApi::kOpenGL)
    {
        glfwShowWindow(impl->handle);
    }

    glfwDefaultWindowHints();

    return {};
}

void sgf::platform::Window::destroy() noexcept
{
    if(impl_ && impl_->handle)
    {
        glfwDestroyWindow(impl_->handle);
        impl_->handle = {};
    }
}

void sgf::platform::Window::setFocused(bool focused) noexcept
{
    if(focused)
    {
        platform().focused_window_ = this;
    }
    else if(platform().focused_window_ == this)
    {
        platform().focused_window_ = {};
    }
}

void sgf::platform::Window::adoptNativeOwner(Window& from) noexcept
{
    if(platform().focused_window_ == &from)
    {
        platform().focused_window_ = this;
    }

    if(impl_ && impl_->handle)
    {
        glfwSetWindowUserPointer(impl_->handle, this);
    }
}

void sgf::platform::Window::Impl::closeCallback(GLFWwindow* window) noexcept
{
    auto* my_window = static_cast<Window*>(glfwGetWindowUserPointer(window));
}

void sgf::platform::Window::Impl::iconifyCallback(GLFWwindow* window, int iconified) noexcept
{
    auto* my_window = static_cast<Window*>(glfwGetWindowUserPointer(window));
}

void sgf::platform::Window::Impl::framebufferSizeCallback(GLFWwindow* window, int width, int height) noexcept
{
    auto* my_window = static_cast<Window*>(glfwGetWindowUserPointer(window));
}

void sgf::platform::Window::Impl::windowFocusedCallback(GLFWwindow* window, int focused) noexcept
{
    auto* my_window = static_cast<Window*>(glfwGetWindowUserPointer(window));
    my_window->setFocused(focused == GLFW_TRUE);
}

void sgf::platform::Window::Impl::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) noexcept
{
    auto* my_window = static_cast<Window*>(glfwGetWindowUserPointer(window));
}
