//
// Created by stuka on 15.11.2026.
//

#include "sgf/core/utility/mapped_file.h"

#include <limits>
#include <mutex>
#include <utility>

#if SGF_PLATFORM_OS_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif SGF_PLATFORM_OS_LINUX

#endif

sgf::core::MappedFile::~MappedFile()
{
    std::unique_lock lock(mutex_);
    std::ignore = unmap();
}

sgf::core::MappedFile::MappedFile(MappedFile&& other) noexcept
{
    std::unique_lock lock(other.mutex_);
    path_ = std::move(other.path_);
    adopt(other);
}

sgf::core::MappedFile& sgf::core::MappedFile::operator=(MappedFile&& other) noexcept
{
    if(this == &other) return *this;

    std::unique_lock lock(mutex_, std::defer_lock);
    std::unique_lock other_lock(other.mutex_, std::defer_lock);
    std::lock(lock, other_lock);

    std::ignore = unmap();
    path_ = std::move(other.path_);
    adopt(other);
    return *this;
}

std::error_code sgf::core::MappedFile::map(std::ios_base::openmode mode)
{
#if SGF_PLATFORM_OS_WINDOWS
    if(path_.empty()) return std::make_error_code(std::errc::invalid_argument);

    const bool read = (mode & std::ios::in) == std::ios::in;
    const bool write = (mode & std::ios::out) == std::ios::out;

    if(!read && !write) return std::make_error_code(std::errc::invalid_argument);

    DWORD access = GENERIC_READ;
    DWORD share_mode = FILE_SHARE_READ;
    DWORD creation = OPEN_EXISTING;
    DWORD protection = PAGE_READONLY;
    DWORD view_access = FILE_MAP_READ;

    if(write)
    {
        // a writable view requires the file to be opened for reading as well
        access = GENERIC_READ | GENERIC_WRITE;
        share_mode = 0;
        protection = PAGE_READWRITE;
        view_access = FILE_MAP_WRITE;

        if((mode & std::ios::noreplace) == std::ios::noreplace) creation = CREATE_NEW;
        else if((mode & std::ios::trunc) == std::ios::trunc) creation = CREATE_ALWAYS;
        else if(!read) creation = OPEN_ALWAYS;
    }

    const HANDLE file = ::CreateFileW(
        path_.c_str(),
        access,
        share_mode,
        nullptr,
        creation,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
        nullptr);
    if(file == INVALID_HANDLE_VALUE)
    {
        return { static_cast<int>(::GetLastError()), std::system_category() };
    }

    LARGE_INTEGER file_size {};
    if(!::GetFileSizeEx(file, &file_size))
    {
        const auto error = ::GetLastError();
        ::CloseHandle(file);
        return { static_cast<int>(error), std::system_category() };
    }

    if(file_size.QuadPart == 0)
    {
        ::CloseHandle(file);
        return {};
    }

    if(static_cast<std::uint64_t>(file_size.QuadPart) > std::numeric_limits<std::size_t>::max())
    {
        ::CloseHandle(file);
        return std::make_error_code(std::errc::file_too_large);
    }

    const HANDLE mapping = ::CreateFileMappingW(file, nullptr, protection, 0, 0, nullptr);
    ::CloseHandle(file);
    if(!mapping)
    {
        return { static_cast<int>(::GetLastError()), std::system_category() };
    }

    void* const view = ::MapViewOfFile(mapping, view_access, 0, 0, 0);
    if(!view)
    {
        const auto error = ::GetLastError();
        ::CloseHandle(mapping);
        return { static_cast<int>(error), std::system_category() };
    }

    view_ = view;
    file_mapping_ = mapping;
    size_ = static_cast<std::size_t>(file_size.QuadPart);
    return {};
#elif SGF_PLATFORM_OS_LINUX
    // todo: mmap
    static_cast<void>(mode);
#endif

    return {};
}

std::error_code sgf::core::MappedFile::unmap()
{
#if SGF_PLATFORM_OS_WINDOWS
    std::error_code error;
    if(view_)
    {
        if(!::UnmapViewOfFile(view_))
        {
            error = { static_cast<int>(::GetLastError()), std::system_category() };
        }
        view_ = {};
    }

    if(file_mapping_)
    {
        if(!::CloseHandle(static_cast<HANDLE>(file_mapping_)) && !error)
        {
            error = { static_cast<int>(::GetLastError()), std::system_category() };
        }
        file_mapping_ = {};
    }

    size_ = 0;
    return error;
#else
    return {};
#endif
}

void sgf::core::MappedFile::adopt(MappedFile& other) noexcept
{
#if SGF_PLATFORM_OS_WINDOWS
    view_ = std::exchange(other.view_, nullptr);
    file_mapping_ = std::exchange(other.file_mapping_, nullptr);
    size_ = std::exchange(other.size_, 0);
#else
    static_cast<void>(other);
#endif
}

std::error_code sgf::core::MappedFile::open(std::ios_base::openmode mode)
{
    std::unique_lock lock(mutex_);
#if SGF_PLATFORM_OS_WINDOWS
    if(view_) return {};
#endif
    return map(mode);
}

std::error_code sgf::core::MappedFile::close()
{
    std::unique_lock lock(mutex_);
    return unmap();
}

bool sgf::core::MappedFile::opened() const
{
    std::shared_lock lock(mutex_);
#if SGF_PLATFORM_OS_WINDOWS
    return view_ != nullptr;
#else
    return false;
#endif
}

std::span<const std::byte> sgf::core::MappedFile::bytes() const
{
    std::shared_lock lock(mutex_);
#if SGF_PLATFORM_OS_WINDOWS
    if(!view_) return {};
    return { static_cast<std::byte const*>(view_), size_ };
#else
    return {};
#endif
}

std::filesystem::path sgf::core::MappedFile::path() const
{
    std::shared_lock lock(mutex_);
    return path_;
}
