//
// Created by stuka on 05.11.2026.
//

#include "sgf/assets/archive.h"

#include <limits>
#include <mutex>

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

sgf::assets::Archive::~Archive()
{
    std::unique_lock lock(mutex_);
    std::ignore = unmap();
}

sgf::assets::Archive::Archive(Archive&& other) noexcept
{
    std::unique_lock lock(other.mutex_);
    path_ = std::move(other.path_);

#if SGF_PLATFORM_OS_WINDOWS
    view_ = other.view_;
    mapping_ = other.mapping_;
    size_ = other.size_;
    other.view_ = nullptr;
    other.mapping_ = nullptr;
    other.size_ = 0;
#endif
}

sgf::assets::Archive& sgf::assets::Archive::operator=(Archive&& other) noexcept
{
    if(this == &other) return *this;

    std::unique_lock lock(mutex_, std::defer_lock);
    std::unique_lock other_lock(other.mutex_, std::defer_lock);
    std::lock(lock, other_lock);

    std::ignore = unmap();
    path_ = std::move(other.path_);

#if SGF_PLATFORM_OS_WINDOWS
    view_ = other.view_;
    mapping_ = other.mapping_;
    size_ = other.size_;
    other.view_ = nullptr;
    other.mapping_ = nullptr;
    other.size_ = 0;
#endif

    return *this;
}

bool sgf::assets::Archive::hasView() const noexcept
{
#if SGF_PLATFORM_OS_WINDOWS
    return view_ != nullptr;
#else
    return false;
#endif
}

std::span<const std::byte> sgf::assets::Archive::mappedBytes() const noexcept
{
#if SGF_PLATFORM_OS_WINDOWS
    return { static_cast<std::byte const*>(view_), size_ };
#else
    return {};
#endif
}

std::error_code sgf::assets::Archive::unmap()
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

    if(mapping_)
    {
        if(!::CloseHandle(static_cast<HANDLE>(mapping_)) && !error)
        {
            error = { static_cast<int>(::GetLastError()), std::system_category() };
        }
        mapping_ = {};
    }

    size_ = 0;
    return error;
#else
    return {};
#endif
}

std::error_code sgf::assets::Archive::open(std::ios_base::openmode mode)
{
    std::unique_lock lock(mutex_);
    if(hasView()) return {};

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

    mapping_ = mapping;
    view_ = view;
    size_ = static_cast<std::size_t>(file_size.QuadPart);

    return {};
#elif SGF_PLATFORM_OS_LINUX
    // todo: mmap
    static_cast<void>(mode);
#endif

    return {};
}

std::error_code sgf::assets::Archive::close()
{
    std::unique_lock lock(mutex_);
    return unmap();
}

bool sgf::assets::Archive::opened() const
{
    std::shared_lock lock(mutex_);
    return hasView();
}

std::span<const std::byte> sgf::assets::Archive::bytes() const
{
    std::shared_lock lock(mutex_);
    return mappedBytes();
}

std::filesystem::path sgf::assets::Archive::path() const
{
    std::shared_lock lock(mutex_);
    return path_;
}
