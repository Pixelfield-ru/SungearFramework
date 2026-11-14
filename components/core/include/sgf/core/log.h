//
// Created by stuka on 13.10.2026.
//

#pragma once

#include <format>
#include <functional>
#include <source_location>
#include <string_view>
#include <type_traits>
#include <utility>

namespace sgf::core
{
    enum class LogLevel
    {
        kTrace,
        kDebug,
        kInfo,
        kWarn,
        kError,
        kFatal
    };

    template<typename... Args>
    struct FormatMessage final
    {
        std::format_string<Args...> fmt;
        std::source_location location;

        template<typename T>
        consteval FormatMessage(const T& text, std::source_location caller = std::source_location::current()) :
            fmt(text),
            location(caller)
        { }
    };

    struct LogCallback final
    {
        bool enabled {};
        std::function<void(LogLevel, std::string_view, std::source_location)> function;

        void operator()(LogLevel level, std::string_view msg,
                        std::source_location caller = std::source_location::current()) const noexcept
        {
            if(!enabled || !function) return;

            try
            {
                function(level, msg, caller);
            }
            catch(...)
            { }
        }

        static void print(const LogCallback* output, LogLevel level, std::string_view msg,
                          std::source_location caller = std::source_location::current())
        {
            if(output)
            {
                (*output)(level, msg, caller);
            }
        }

        /// Formats at the call site and swallows allocation or callback failures.
        template<typename... Args>
        requires(sizeof...(Args) > 0)
        static void print(const LogCallback* output, LogLevel level,
                          FormatMessage<std::type_identity_t<Args>...> fmt, Args&&... args) noexcept
        {
            try
            {
                if(!output) return;

                (*output)(level, std::format(fmt.fmt, std::forward<Args>(args)...), fmt.location);
            }
            catch(...)
            { }
        }

        static void trace(const LogCallback* output, std::string_view msg,
                          std::source_location caller = std::source_location::current())
        {
            print(output, LogLevel::kTrace, msg, caller);
        }

        template<typename... Args>
        requires(sizeof...(Args) > 0)
        static void trace(const LogCallback* output, FormatMessage<std::type_identity_t<Args>...> fmt,
                          Args&&... args) noexcept
        {
            print(output, LogLevel::kTrace, fmt, std::forward<Args>(args)...);
        }

        static void debug(const LogCallback* output, std::string_view msg,
                          std::source_location caller = std::source_location::current())
        {
            print(output, LogLevel::kDebug, msg, caller);
        }

        template<typename... Args>
        requires(sizeof...(Args) > 0)
        static void debug(const LogCallback* output, FormatMessage<std::type_identity_t<Args>...> fmt,
                          Args&&... args) noexcept
        {
            print(output, LogLevel::kDebug, fmt, std::forward<Args>(args)...);
        }

        static void info(const LogCallback* output, std::string_view msg,
                         std::source_location caller = std::source_location::current())
        {
            print(output, LogLevel::kInfo, msg, caller);
        }

        template<typename... Args>
        requires(sizeof...(Args) > 0)
        static void info(const LogCallback* output, FormatMessage<std::type_identity_t<Args>...> fmt,
                         Args&&... args) noexcept
        {
            print(output, LogLevel::kInfo, fmt, std::forward<Args>(args)...);
        }

        static void warn(const LogCallback* output, std::string_view msg,
                         std::source_location caller = std::source_location::current())
        {
            print(output, LogLevel::kWarn, msg, caller);
        }

        template<typename... Args>
        requires(sizeof...(Args) > 0)
        static void warn(const LogCallback* output, FormatMessage<std::type_identity_t<Args>...> fmt,
                         Args&&... args) noexcept
        {
            print(output, LogLevel::kWarn, fmt, std::forward<Args>(args)...);
        }

        static void error(const LogCallback* output, std::string_view msg,
                          std::source_location caller = std::source_location::current())
        {
            print(output, LogLevel::kError, msg, caller);
        }

        template<typename... Args>
        requires(sizeof...(Args) > 0)
        static void error(const LogCallback* output, FormatMessage<std::type_identity_t<Args>...> fmt,
                          Args&&... args) noexcept
        {
            print(output, LogLevel::kError, fmt, std::forward<Args>(args)...);
        }

        static void fatal(const LogCallback* output, std::string_view msg,
                          std::source_location caller = std::source_location::current())
        {
            print(output, LogLevel::kFatal, msg, caller);
        }

        template<typename... Args>
        requires(sizeof...(Args) > 0)
        static void fatal(const LogCallback* output, FormatMessage<std::type_identity_t<Args>...> fmt,
                          Args&&... args) noexcept
        {
            print(output, LogLevel::kFatal, fmt, std::forward<Args>(args)...);
        }
    };
}
