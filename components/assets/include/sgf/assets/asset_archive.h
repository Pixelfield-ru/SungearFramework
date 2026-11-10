//
// Created by stuka on 05.11.2026.
//

#pragma once

#include <filesystem>
#include <ios>
#include <span>
#include <system_error>

#include "sgf/core/utility/platform.h"

namespace sgf::assets
{
    // todo: linux & macos impl
    struct Archive final
    {
        ~Archive();

        explicit Archive(std::filesystem::path path) :
            path_(std::move(path))
        { }
        Archive(const Archive&) = delete;
        Archive(Archive&&) noexcept;

        [[nodiscard]] std::error_code open(std::ios_base::openmode mode);
        [[nodiscard]] std::error_code close();

        [[nodiscard]] bool opened() const noexcept;

        [[nodiscard]] std::span<const std::byte> bytes() const noexcept;

        [[nodiscard]] std::filesystem::path path() const noexcept;

        Archive& operator=(const Archive&) = delete;
        Archive& operator=(Archive&&) noexcept;

    private:
        std::filesystem::path path_;

#if SGF_PLATFORM_OS_WINDOWS
        void* view_ {};
        void* mapping_ {};
        std::size_t size_ {};
#endif
    };
}