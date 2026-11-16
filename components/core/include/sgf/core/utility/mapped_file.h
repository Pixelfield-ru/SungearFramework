//
// Created by stuka on 15.11.2026.
//

#pragma once

#include <expected>
#include <filesystem>
#include <ios>
#include <shared_mutex>
#include <span>
#include <system_error>

#include "sgf/core/utility/platform.h"
#include "sgf/core//memory.h"

namespace sgf::core
{
    // todo: linux & macos impl
    struct MappedFile final
    {
        ~MappedFile();

        MappedFile() = default;
        explicit MappedFile(std::filesystem::path path) :
            path_(std::move(path))
        { }
        MappedFile(const MappedFile&) = delete;
        MappedFile(MappedFile&&) noexcept;

        [[nodiscard]] std::error_code open(std::ios_base::openmode mode);
        /// Unmaps the file. Do not call while a load or a read of bytes() is in progress.
        [[nodiscard]] std::error_code close();

        [[nodiscard]] bool opened() const;

        /// Bytes of the open mapping. Empty when the file is closed.
        /// Valid until close(). Do not close the file while the span is in use.
        [[nodiscard]] std::span<const std::byte> bytes() const;

        [[nodiscard]] std::size_t size() const noexcept;

        [[nodiscard]] std::expected<Vector<std::byte>, std::error_code> copyBytes(
            std::size_t offset, std::size_t size,
            const std::pmr::polymorphic_allocator<>& alloc = { }) const;

        [[nodiscard]] std::filesystem::path path() const;

        MappedFile& operator=(const MappedFile&) = delete;
        MappedFile& operator=(MappedFile&&) noexcept;

    private:
        /// Maps path_. Members stay unchanged until the whole mapping succeeds.
        [[nodiscard]] std::error_code map(std::ios_base::openmode mode);
        [[nodiscard]] std::error_code unmap();
        void adopt(MappedFile& other) noexcept;

        std::filesystem::path path_;
        mutable std::shared_mutex mutex_;

#if SGF_PLATFORM_OS_WINDOWS
        void* view_ {};
        void* file_mapping_ {};
        std::size_t size_ {};
#endif
    };
}
