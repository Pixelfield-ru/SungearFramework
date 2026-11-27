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
     * Copies the contents of a directory.
     * An existing file stays in place when skip_existing is true, and is replaced when it is false.
     * A directory symlink or a non-regular file returns not_supported. Files copied before a failure stay on disk.
     * Returns invalid_argument when either path is the same as the other or lies inside it.
     * @param from Source directory.
     * @param to Destination directory. Created when missing.
     * @param skip_existing Leave files that are already at the destination.
     * @return Empty error code on success.
     */
    [[nodiscard]] std::error_code copyDirectory(
        const std::filesystem::path& from,
        const std::filesystem::path& to,
        bool skip_existing);

    /**
     * True when child is parent or a path inside parent.
     * Resolves the existing parts of both paths. On Windows the comparison ignores case.
     * @param child Path that may lie inside parent.
     * @param parent Directory to test against.
     * @return True when child is parent or nested in it.
     */
    [[nodiscard]] std::expected<bool, std::error_code> sameOrNested(
        const std::filesystem::path& child,
        const std::filesystem::path& parent);

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

    /**
     * @param extension Source extension.
     * @param alloc Allocator.
     * @return Lowered extension.
     */
    [[nodiscard]] String normalizedExtension(std::string_view extension, const std::pmr::polymorphic_allocator<char>& alloc = {});
}
