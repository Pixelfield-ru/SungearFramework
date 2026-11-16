//
// Created by stuka on 16.11.2026.
//

#pragma once

#include <optional>

#include "sgf/core/log.h"
#include "sgf/platform/types.h"

namespace sgf::platform
{
    [[nodiscard]] int toGlfwKey(KeyboardKey key) noexcept;
    [[nodiscard]] int toGlfwMouseButton(MouseButton button) noexcept;
    [[nodiscard]] int toGlfwKeyMod(KeyMod mods) noexcept;
    [[nodiscard]] int toGlfwKeyAction(KeyAction action) noexcept;

    [[nodiscard]] KeyboardKey fromGlfwKey(int glfw_key) noexcept;
    [[nodiscard]] std::optional<MouseButton> fromGlfwMouseButton(int glfw_button) noexcept;
    [[nodiscard]] KeyMod fromGlfwKeyMod(int glfw_mods) noexcept;
    [[nodiscard]] std::optional<KeyAction> fromGlfwKeyAction(int glfw_action) noexcept;

    [[nodiscard]] core::LogLevel fromGlfwError(int glfw_error) noexcept;
}
