//
// Created by stuka on 05.11.2026.
//

#pragma once

#include <filesystem>
#include <ios>
#include <shared_mutex>
#include <span>
#include <system_error>

#include "sgf/core/utility/platform.h"

namespace sgf::assets
{
    struct AssetHeader final
    {
        std::uint64_t content_block_size {};
        std::uint64_t dependencies_block_size {};
    };

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

        [[nodiscard]] bool opened() const;

        [[nodiscard]] std::span<const std::byte> bytes() const;

        [[nodiscard]] std::filesystem::path path() const;

        Archive& operator=(const Archive&) = delete;
        Archive& operator=(Archive&&) noexcept;

    private:
        // Not thread-safe.
        [[nodiscard]] bool hasView() const noexcept;
        // Not thread-safe.
        [[nodiscard]] std::span<const std::byte> mappedBytes() const noexcept;
        // Not thread-safe.
        [[nodiscard]] std::error_code unmap();

        std::filesystem::path path_;
        mutable std::shared_mutex mutex_;

#if SGF_PLATFORM_OS_WINDOWS
        void* view_ {};
        void* mapping_ {};
        std::size_t size_ {};
#endif
    };
}