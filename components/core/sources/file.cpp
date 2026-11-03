//
// Created by stuka on 03.11.2026.
//

#include "sgf/core/utility/file.h"

#include <fstream>
#include <limits>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace
{
    std::error_code replaceFile(const std::filesystem::path& from, const std::filesystem::path& to)
    {
#if defined(_WIN32)
        if(::MoveFileExW(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        {
            return {};
        }

        return { static_cast<int>(::GetLastError()), std::system_category() };
#else
        std::error_code error;
        std::filesystem::rename(from, to, error);
        return error;
#endif
    }
}

std::expected<sgf::core::Vector<std::byte>, std::error_code> sgf::core::readFile(
    const std::filesystem::path& path,
    const std::pmr::polymorphic_allocator<>& alloc)
{
    if(path.empty()) return std::unexpected(std::make_error_code(std::errc::invalid_argument));

    std::error_code error;
    const auto file_size = std::filesystem::file_size(path, error);
    if(error) return std::unexpected(error);

    constexpr auto max_size = static_cast<std::uintmax_t>(std::numeric_limits<std::streamsize>::max());
    if(file_size > max_size) return std::unexpected(std::make_error_code(std::errc::file_too_large));

    Vector bytes(alloc);
    bytes.resize(file_size);
    if(bytes.empty()) return bytes;

    std::ifstream stream(path, std::ios::binary);
    if(!stream) return std::unexpected(std::make_error_code(std::errc::io_error));

    stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if(stream.gcount() != static_cast<std::streamsize>(bytes.size()))
    {
        return std::unexpected(std::make_error_code(std::errc::io_error));
    }

    return bytes;
}

std::error_code sgf::core::writeFile(const std::filesystem::path& path, std::span<const std::byte> bytes)
{
    if(path.empty() || !path.has_filename()) return std::make_error_code(std::errc::invalid_argument);
    if(bytes.size() > static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max()))
    {
        return std::make_error_code(std::errc::file_too_large);
    }

    const auto directory = path.parent_path();
    std::error_code error;
    if(!directory.empty())
    {
        std::filesystem::create_directories(directory, error);
        if(error) return error;
    }

    auto temp_path = path;
    temp_path.concat(".tmp");

    {
        std::ofstream stream(temp_path, std::ios::binary | std::ios::trunc);
        if(!stream) return std::make_error_code(std::errc::io_error);

        if(!bytes.empty())
        {
            stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        }

        stream.close();
        if(stream.fail())
        {
            std::filesystem::remove(temp_path, error);
            return std::make_error_code(std::errc::io_error);
        }
    }

    if(const auto replace_error = replaceFile(temp_path, path))
    {
        std::filesystem::remove(temp_path, error);
        return replace_error;
    }

    return {};
}

std::expected<std::filesystem::path, std::error_code> sgf::core::normalizedRelativePath(
    const std::filesystem::path& path,
    const std::filesystem::path& root)
{
    if(path.empty() || root.empty()) return std::unexpected(std::make_error_code(std::errc::invalid_argument));

    auto relative = path.lexically_normal();
    if(relative.is_absolute()) relative = relative.lexically_relative(root.lexically_normal());

    relative = relative.lexically_normal();
    if(relative.empty() || relative.has_root_name() || relative.has_root_directory() || !relative.has_filename())
    {
        return std::unexpected(std::make_error_code(std::errc::invalid_argument));
    }

    for(const auto& part : relative)
    {
        if(part == "..") return std::unexpected(std::make_error_code(std::errc::invalid_argument));
    }

    return std::filesystem::path { relative.generic_string() };
}
