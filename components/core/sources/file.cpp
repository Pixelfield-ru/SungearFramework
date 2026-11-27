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
    bool samePathElement(const std::filesystem::path& left, const std::filesystem::path& right)
    {
#if defined(_WIN32)
        return ::CompareStringOrdinal(left.c_str(), -1, right.c_str(), -1, TRUE) == CSTR_EQUAL;
#else
        return left == right;
#endif
    }

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

    Vector<std::byte> bytes(alloc);
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

std::error_code sgf::core::copyDirectory(
    const std::filesystem::path& from,
    const std::filesystem::path& to,
    bool skip_existing)
{
    std::error_code error;
    if(!std::filesystem::is_directory(from, error))
    {
        if(!error) error = std::make_error_code(std::errc::not_a_directory);
        return error;
    }

    const auto destination_inside_source = sameOrNested(to, from);
    if(!destination_inside_source) return destination_inside_source.error();
    const auto source_inside_destination = sameOrNested(from, to);
    if(!source_inside_destination) return source_inside_destination.error();
    if(*destination_inside_source || *source_inside_destination)
    {
        return std::make_error_code(std::errc::invalid_argument);
    }

    std::filesystem::create_directories(to, error);
    if(error) return error;

    std::filesystem::recursive_directory_iterator cursor(from, error);
    if(error) return error;

    const auto file_options = skip_existing
        ? std::filesystem::copy_options::skip_existing
        : std::filesystem::copy_options::overwrite_existing;
    const std::filesystem::recursive_directory_iterator end;

    while(cursor != end)
    {
        const auto& entry = *cursor;
        const auto relative = entry.path().lexically_relative(from);
        if(relative.empty() || relative.is_absolute() || *relative.begin() == "..")
        {
            return std::make_error_code(std::errc::invalid_argument);
        }

        const auto target = to / relative;
        const bool symlink = entry.is_symlink(error);
        if(error) return error;
        const bool directory = entry.is_directory(error);
        if(error) return error;

        if(directory)
        {
            if(symlink) return std::make_error_code(std::errc::not_supported);
            std::filesystem::create_directories(target, error);
            if(error) return error;
        }
        else
        {
            const bool regular = entry.is_regular_file(error);
            if(error) return error;
            if(!regular) return std::make_error_code(std::errc::not_supported);

            std::filesystem::copy_file(entry.path(), target, file_options, error);
            if(error) return error;
        }

        cursor.increment(error);
        if(error) return error;
    }

    return {};
}

std::expected<bool, std::error_code> sgf::core::sameOrNested(
    const std::filesystem::path& child,
    const std::filesystem::path& parent)
{
    std::error_code error;
    const auto child_path = std::filesystem::weakly_canonical(child, error);
    if(error) return std::unexpected(error);
    const auto parent_path = std::filesystem::weakly_canonical(parent, error);
    if(error) return std::unexpected(error);

    auto parent_it = parent_path.begin();
    auto child_it = child_path.begin();
    for(; parent_it != parent_path.end(); ++parent_it, ++child_it)
    {
        if(child_it == child_path.end() || !samePathElement(*parent_it, *child_it)) return false;
    }

    return true;
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

sgf::core::String sgf::core::normalizedExtension(std::string_view extension,
                                                 const std::pmr::polymorphic_allocator<char>& alloc)
{
    String key(extension, alloc);
    for(char& ch : key)
    {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }

    return key;
}
