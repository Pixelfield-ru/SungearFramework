//
// Created by stuka on 03.11.2026.
//

#pragma once

#include <expected>
#include <filesystem>
#include <span>
#include <system_error>

#include "sgf/core/memory.h"

namespace sgf::core
{
    /**
     * Reads the whole file into memory.
     * @param path File path.
     * @param alloc Allocator for the resulting buffer.
     * @return File bytes, or an error code.
     */
    [[nodiscard]] std::expected<Vector<std::byte>, std::error_code> readFile(
        const std::filesystem::path& path,
        const std::pmr::polymorphic_allocator<>& alloc);

    /**я
     * Writes bytes to a temporary file in the same directory, then replaces the destination.
     * @param path Destination path.
     * @param bytes Bytes to write.
     * @return Empty error code on success.
     */
    [[nodiscard]] std::error_code writeFile(const std::filesystem::path& path, std::span<const std::byte> bytes);

    /**
     * Makes a relative lexical path under root.
     * An absolute path is made relative to root. Internal `.`, duplicate separators and cancelling `..` are removed.
     * A path that resolves to root itself becomes `.`.
     * Does not touch the filesystem and does not change case.
     * @param path Path to normalize.
     * @param root Directory the result must stay inside.
     * @return Relative path with generic separators, or an error if it is empty or escapes root.
     */
    [[nodiscard]] std::expected<std::filesystem::path, std::error_code> normalizedRelativePath(
        const std::filesystem::path& path,
        const std::filesystem::path& root);
}
