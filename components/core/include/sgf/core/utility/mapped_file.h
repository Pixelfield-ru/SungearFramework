//
// Created by stuka on 15.11.2026.
//

#pragma once

#include <filesystem>
#include <ios>
#include <shared_mutex>
#include <span>
#include <system_error>

#include "sgf/core/memory.h"
#include "sgf/core/utility/platform.h"

namespace sgf::core
{
    // todo: linux & macos impl
    struct MappedFile final
    {
        using allocator_type = std::pmr::polymorphic_allocator<>;

    private:
        /// One OS mapping. Lives until the last Access is destroyed.
        struct Mapping final
        {
            Mapping() = default;
            Mapping(Mapping&&) noexcept;
            ~Mapping();

            Mapping(const Mapping&) = delete;
            Mapping& operator=(const Mapping&) = delete;
            Mapping& operator=(Mapping&&) = delete;

            /// Empty file leaves this mapping without a view.
            [[nodiscard]] std::error_code map(const std::filesystem::path& path, std::ios_base::openmode mode);

            [[nodiscard]] std::span<const std::byte> bytes() const noexcept;
            [[nodiscard]] std::error_code unmap();

#if SGF_PLATFORM_OS_WINDOWS
            void* view_ {};
            void* file_mapping_ {};
            std::size_t size_ {};
#endif
        };

    public:
        /// Bytes of the current mapping. Holds that mapping until the last copy is destroyed.
        struct Access final
        {
            [[nodiscard]] std::span<const std::byte> bytes() const noexcept
            {
                return bytes_;
            }

            /// Is mapping valid.
            [[nodiscard]] explicit operator bool() const noexcept
            {
                return static_cast<bool>(mapping_);
            }

        private:
            friend struct MappedFile;

            std::span<const std::byte> bytes_ {};
            SharedPtr<Mapping> mapping_;
        };

        ~MappedFile();

        explicit MappedFile(std::filesystem::path path, const allocator_type& allocator = {}) :
            path_(std::move(path)),
            resource_(allocator.resource())
        { }
        MappedFile(const MappedFile&) = delete;
        MappedFile(MappedFile&&) noexcept;

        [[nodiscard]] std::error_code open(std::ios_base::openmode mode);
        [[nodiscard]] std::error_code close();

        [[nodiscard]] bool opened() const;

        /// Copies the current mapping. The result stays valid after close() and destruction.
        [[nodiscard]] Access access() const;

        [[nodiscard]] std::filesystem::path path() const;

        MappedFile& operator=(const MappedFile&) = delete;
        MappedFile& operator=(MappedFile&&) noexcept;

    private:
        std::filesystem::path path_;
        std::pmr::memory_resource* resource_ {};
        mutable std::shared_mutex mutex_;
        SharedPtr<Mapping> mapping_;
    };
}
