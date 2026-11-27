//
// Created by stuka on 25.11.2026.
//

#pragma once

#include <filesystem>
#include <string_view>
#include <system_error>

#include "sgf/core/memory.h"

namespace sgf::core
{
    /**
     * Values loaded from a .env file.
     * get() reads the process environment first, then the file, then returns an empty string.
     */
    struct Env final
    {
        using allocator_type = std::pmr::polymorphic_allocator<>;

        explicit Env(const allocator_type& alloc = {}) :
            values_(alloc)
        { }

        /**
         * Replace the stored file values with the contents of a .env file.
         * A line is KEY=VALUE. Empty lines and lines that start with # are ignored.
         * export KEY=VALUE is accepted. The process environment is not read here.
         * @param path Path to the .env file.
         * @return Empty error code on success.
         */
        std::error_code load(const std::filesystem::path& path);

        /**
         * Process environment, then the loaded file, then an empty string.
         * An empty process value stays empty and does not fall through to the file.
         * @param key Variable name.
         * @return Variable value.
         */
        [[nodiscard]] String get(std::string_view key) const;

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return values_.get_allocator();
        }

    private:
        UnorderedMap<String, String> values_;
    };
}
