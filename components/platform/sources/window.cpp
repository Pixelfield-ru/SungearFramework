//
// Created by stuka on 13.10.2026.
//

#include "sgf/platform/window.h"

#include <memory>
#include <GLFW/glfw3.h>

#include "sgf/core/errors.h"
#include "sgf/platform/glfw/glfw_errors.h"
#include "sgf/platform/glfw/glfw_types.h"
#include "sgf/platform/platform.h"

struct sgf::platform::Window::Impl
{
    GLFWwindow* handle {};

    static void closeCallback(GLFWwindow* window) noexcept;
    static void iconifyCallback(GLFWwindow* window, int iconified) noexcept;
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height) noexcept;
    static void windowFocusedCallback(GLFWwindow* window, int focused);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) noexcept;
    static void charCallback(GLFWwindow* window, unsigned int codepoint) noexcept;
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) noexcept;
    static void cursorPosCallback(GLFWwindow* window, double x, double y) noexcept;
    static void scrollCallback(GLFWwindow* window, double x, double y) noexcept;
    static void cursorEnterCallback(GLFWwindow* window, int entered) noexcept;
};

sgf::platform::Window::Window(const allocator_type& alloc) noexcept :
    resource_(alloc.resource()),
    impl_(core::makeUnique<Impl>(alloc)),
    keyboard_(alloc),
    mouse_(alloc)
{
    platform().addWindow(this);
}

sgf::platform::Window::Window(Window&& other) noexcept :
    resource_(other.resource_),
    impl_(std::move(other.impl_)),
    debug_output_(other.debug_output_),
    keyboard_(std::move(other.keyboard_)),
    mouse_(std::move(other.mouse_))
{
    platform().addWindow(this);
    adoptNativeOwner(other);
}

sgf::platform::Window::Window(Window&& other, const allocator_type& alloc) noexcept :
    resource_(other.resource_),
    impl_(std::move(other.impl_)),
    debug_output_(other.debug_output_),
    keyboard_(std::move(other.keyboard_), alloc),
    mouse_(std::move(other.mouse_), alloc)
{
    platform().addWindow(this);
    adoptNativeOwner(other);
}

sgf::platform::Window::~Window() noexcept
{
    destroy();

    try
    {
        keyboard_.releaseAll();
    }
    catch(...)
    {
    }

    try
    {
        mouse_.releaseAll();
    }
    catch(...)
    {
    }

    platform().removeWindow(this);
}

void sgf::platform::Window::setTitle(std::string_view title) noexcept
{
    if(!valid()) return;
    glfwSetWindowTitle(impl_->handle, title.data());
}

std::string_view sgf::platform::Window::title() const noexcept
{
    if(!valid()) return {};

    const char* title = glfwGetWindowTitle(impl_->handle);
    return title ? std::string_view{ title } : std::string_view{};
}

void sgf::platform::Window::position(std::int32_t& x, std::int32_t& y) const noexcept
{
    if(!valid())
    {
        x = 0;
        y = 0;
        return;
    }
    glfwGetWindowPos(impl_->handle, &x, &y);
}

void sgf::platform::Window::setPosition(std::int32_t x, std::int32_t y) noexcept
{
    if(!valid()) return;
    glfwSetWindowPos(impl_->handle, x, y);
}

void sgf::platform::Window::size(std::int32_t& width, std::int32_t& height) const noexcept
{
    if(!valid())
    {
        width = 0;
        height = 0;
        return;
    }
    glfwGetWindowSize(impl_->handle, &width, &height);
}

void sgf::platform::Window::framebufferSize(std::int32_t& width, std::int32_t& height) const noexcept
{
    if(!valid())
    {
        width = 0;
        height = 0;
        return;
    }
    glfwGetFramebufferSize(impl_->handle, &width, &height);
}

void sgf::platform::Window::setSize(std::int32_t width, std::int32_t height) noexcept
{
    if(!valid()) return;
    glfwSetWindowSize(impl_->handle, width, height);
}

void sgf::platform::Window::close() noexcept
{
    if(!valid()) return;

    try
    {
        keyboard_.releaseAll();
    }
    catch(...)
    {
    }

    try
    {
        mouse_.releaseAll();
    }
    catch(...)
    {
    }

    glfwDestroyWindow(impl_->handle);
    impl_->handle = {};

    setFocused(false);

    --platform().opened_windows_count_;
}

bool sgf::platform::Window::shouldClose() const noexcept
{
    if(!valid()) return false;
    return glfwWindowShouldClose(impl_->handle) == GLFW_TRUE;
}

bool sgf::platform::Window::closed() const noexcept
{
    return !valid();
}

const sgf::platform::Keyboard& sgf::platform::Window::keyboard() const noexcept
{
    return keyboard_;
}

sgf::platform::Keyboard& sgf::platform::Window::keyboard() noexcept
{
    return keyboard_;
}

const sgf::platform::Mouse& sgf::platform::Window::mouse() const noexcept
{
    return mouse_;
}

sgf::platform::Mouse& sgf::platform::Window::mouse() noexcept
{
    return mouse_;
}

void* sgf::platform::Window::handle() const noexcept
{
    if(!valid()) return nullptr;
    return impl_->handle;
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
    keyboard_ = std::move(other.keyboard_);
    mouse_ = std::move(other.mouse_);

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
        core::LogCallback::error(output.debug_output_, "Cannot create GLFW window.");
        return GLFWError::kCannotCreateWindow;
    }

    glfwSetWindowUserPointer(impl->handle, &output);
    glfwSetWindowCloseCallback(impl->handle, &Impl::closeCallback);
    glfwSetWindowIconifyCallback(impl->handle, &Impl::iconifyCallback);
    glfwSetFramebufferSizeCallback(impl->handle, &Impl::framebufferSizeCallback);
    glfwSetWindowFocusCallback(impl->handle, &Impl::windowFocusedCallback);
    glfwSetKeyCallback(impl->handle, &Impl::keyCallback);
    glfwSetCharCallback(impl->handle, &Impl::charCallback);
    glfwSetMouseButtonCallback(impl->handle, &Impl::mouseButtonCallback);
    glfwSetCursorPosCallback(impl->handle, &Impl::cursorPosCallback);
    glfwSetScrollCallback(impl->handle, &Impl::scrollCallback);
    glfwSetCursorEnterCallback(impl->handle, &Impl::cursorEnterCallback);

    double cursor_x {};
    double cursor_y {};
    glfwGetCursorPos(impl->handle, &cursor_x, &cursor_y);
    output.mouse_.pushMove(cursor_x, cursor_y);
    output.mouse_.setInside(glfwGetWindowAttrib(impl->handle, GLFW_HOVERED) == GLFW_TRUE);

    if(create_info.client_api == WindowClientApi::kOpenGL)
    {
        glfwShowWindow(impl->handle);
    }

    // The window can receive focus inside glfwCreateWindow, before the callback exists.
    // Clicking that window later does not send a second gain event.
    if(glfwGetWindowAttrib(impl->handle, GLFW_FOCUSED) == GLFW_TRUE)
    {
        output.setFocused(true);
    }

    glfwDefaultWindowHints();

    ++platform().opened_windows_count_;

    return {};
}

bool sgf::platform::Window::valid() const noexcept
{
    return impl_ && impl_->handle;
}

void sgf::platform::Window::destroy() noexcept
{
    if(impl_ && impl_->handle)
    {
        glfwDestroyWindow(impl_->handle);
        impl_->handle = {};

        --platform().opened_windows_count_;
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
        platform().defocusWindow();
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

void sgf::platform::Window::Impl::windowFocusedCallback(GLFWwindow* window, int focused)
{
    auto* my_window = static_cast<Window*>(glfwGetWindowUserPointer(window));
    my_window->setFocused(focused == GLFW_TRUE);
}

void sgf::platform::Window::Impl::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) noexcept
{
    auto* my_window = static_cast<Window*>(glfwGetWindowUserPointer(window));

    const auto my_action = fromGlfwKeyAction(action);
    if(!my_action) return;

    my_window->keyboard_.pushKey(fromGlfwKey(key), scancode, *my_action, fromGlfwKeyMod(mods));
}

void sgf::platform::Window::Impl::charCallback(GLFWwindow* window, unsigned int codepoint) noexcept
{
    auto* my_window = static_cast<Window*>(glfwGetWindowUserPointer(window));
    my_window->keyboard_.pushText(codepoint);
}

void sgf::platform::Window::Impl::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) noexcept
{
    auto* my_window = static_cast<Window*>(glfwGetWindowUserPointer(window));

    const auto my_button = fromGlfwMouseButton(button);
    const auto my_action = fromGlfwKeyAction(action);
    if(!my_button || !my_action || *my_action == KeyAction::kRepeat) return;

    my_window->mouse_.pushButton(*my_button, *my_action, fromGlfwKeyMod(mods));
}

void sgf::platform::Window::Impl::cursorPosCallback(GLFWwindow* window, double x, double y) noexcept
{
    auto* my_window = static_cast<Window*>(glfwGetWindowUserPointer(window));
    my_window->mouse_.pushMove(x, y);
}

void sgf::platform::Window::Impl::scrollCallback(GLFWwindow* window, double x, double y) noexcept
{
    auto* my_window = static_cast<Window*>(glfwGetWindowUserPointer(window));
    my_window->mouse_.pushScroll(x, y);
}

void sgf::platform::Window::Impl::cursorEnterCallback(GLFWwindow* window, int entered) noexcept
{
    auto* my_window = static_cast<Window*>(glfwGetWindowUserPointer(window));
    my_window->mouse_.setInside(entered == GLFW_TRUE);
}
