//
// Created by stuka on 05.11.2026.
//

#pragma once

#include <filesystem>
#include <system_error>

namespace sgf::assets
{
    // todo: impl
    struct Archive final
    {
        ~Archive();

        explicit Archive(std::filesystem::path path) :
            path_(std::move(path))
        { }
        Archive(const Archive&) = default;
        Archive(Archive&&) noexcept = default;

        [[nodiscard]] std::error_code open();
        [[nodiscard]] std::error_code close();

        [[nodiscard]] std::filesystem::path path() const noexcept;

        Archive& operator=(const Archive&) = default;
        Archive& operator=(Archive&&) = default;

    private:
        std::filesystem::path path_;
    };
}