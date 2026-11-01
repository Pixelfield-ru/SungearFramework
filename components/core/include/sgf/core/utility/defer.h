//
// Created by stuka on 02.11.2026.
//

#pragma once

#include <functional>

#include "macroses.h"

#define SGF_DEFER(func) sgf::core::Defer SGF_CONCAT(defer_, __LINE__)(func)

namespace sgf::core
{
    struct Defer final
    {
        Defer(std::function<void()> func) noexcept : func_(std::move(func)) {}
        Defer(const Defer&) = delete;
        Defer(Defer&&) noexcept = default;
        ~Defer() noexcept
        {
            func_();
        }

        Defer& operator=(const Defer&) = delete;
        Defer& operator=(Defer&&) = default;

    private:
        std::function<void()> func_;
    };
}