//
// Created by stuka on 25.11.2026.
//

#include "sgf/core/utility/env.h"

#include "sgf/core/utility/encoding.h"
#include "sgf/core/utility/file.h"
#include "sgf/core/utility/platform.h"

#include <cstdlib>
#include <optional>
#include <string>

#if SGF_PLATFORM_OS_WINDOWS
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
    std::string_view trim(std::string_view text)
    {
        while(!text.empty() && (text.front() == ' ' || text.front() == '\t')) text.remove_prefix(1);
        while(!text.empty() && (text.back() == ' ' || text.back() == '\t')) text.remove_suffix(1);
        return text;
    }

    std::error_code parseQuoted(std::string_view text, char quote, std::string& output)
    {
        output.clear();

        while(!text.empty())
        {
            if(text.front() == quote)
            {
                text.remove_prefix(1);
                text = trim(text);
                if(!text.empty()) return std::make_error_code(std::errc::invalid_argument);
                return {};
            }

            if(quote == '"' && text.front() == '\\')
            {
                text.remove_prefix(1);
                if(text.empty()) return std::make_error_code(std::errc::invalid_argument);

                switch(text.front())
                {
                    case 'n': output.push_back('\n'); break;
                    case 'r': output.push_back('\r'); break;
                    case 't': output.push_back('\t'); break;
                    default: output.push_back(text.front()); break;
                }

                text.remove_prefix(1);
                continue;
            }

            output.push_back(text.front());
            text.remove_prefix(1);
        }

        return std::make_error_code(std::errc::invalid_argument);
    }

    std::error_code parseValue(std::string_view text, std::string& output)
    {
        text = trim(text);
        if(text.empty())
        {
            output.clear();
            return {};
        }

        if(text.front() == '"' || text.front() == '\'')
        {
            const char quote = text.front();
            return parseQuoted(text.substr(1), quote, output);
        }

        output.assign(text);
        return {};
    }

    bool validKey(std::string_view key)
    {
        if(key.empty()) return false;

        for(const char ch : key)
        {
            if(ch == ' ' || ch == '\t' || ch == '=') return false;
        }

        return true;
    }

    std::error_code parseLine(std::string_view line, sgf::core::UnorderedMap<sgf::core::String, sgf::core::String>& output)
    {
        line = trim(line);
        if(line.empty() || line.front() == '#') return {};

        constexpr std::string_view prefix = "export";
        if(line.starts_with(prefix) && line.size() > prefix.size() && (line[prefix.size()] == ' ' || line[prefix.size()] == '\t'))
        {
            line = trim(line.substr(prefix.size()));
        }

        const auto separator = line.find('=');
        if(separator == std::string_view::npos) return std::make_error_code(std::errc::invalid_argument);

        const auto key = trim(line.substr(0, separator));
        if(!validKey(key)) return std::make_error_code(std::errc::invalid_argument);

        std::string value;
        if(const auto err = parseValue(line.substr(separator + 1), value)) return err;

        const auto alloc = output.get_allocator();
        output.insert_or_assign(sgf::core::String(key, alloc), sgf::core::String(value, alloc));
        return {};
    }

    std::error_code parseEnv(std::string_view content, sgf::core::UnorderedMap<sgf::core::String, sgf::core::String>& output)
    {
        if(content.starts_with("\xEF\xBB\xBF")) content.remove_prefix(3);

        std::size_t line_start = 0;
        while(line_start <= content.size())
        {
            auto line_end = content.find('\n', line_start);
            if(line_end == std::string_view::npos) line_end = content.size();

            auto line = content.substr(line_start, line_end - line_start);
            if(!line.empty() && line.back() == '\r') line.remove_suffix(1);
            if(const auto err = parseLine(line, output)) return err;

            if(line_end == content.size()) break;
            line_start = line_end + 1;
        }

        return {};
    }

    /// Empty optional means the variable is not set.
    std::optional<std::string> processEnvironment(std::string_view key)
    {
#if SGF_PLATFORM_OS_WINDOWS
        static_assert(sizeof(wchar_t) == sizeof(char16_t));

        const auto utf16 = sgf::core::toUtf16(key);
        if(!utf16) return std::nullopt;

        std::wstring wide(utf16->size(), L'\0');
        for(std::size_t index = 0; index < utf16->size(); ++index)
        {
            wide[index] = static_cast<wchar_t>((*utf16)[index]);
        }

        const DWORD required = ::GetEnvironmentVariableW(wide.c_str(), nullptr, 0);
        if(required == 0) return std::nullopt;

        std::wstring value(required, L'\0');
        const DWORD written = ::GetEnvironmentVariableW(wide.c_str(), value.data(), required);
        if(written == 0 && ::GetLastError() == ERROR_ENVVAR_NOT_FOUND) return std::nullopt;
        value.resize(written);

        const auto units = value.empty()
            ? std::u16string_view {}
            : std::u16string_view(reinterpret_cast<const char16_t*>(value.data()), value.size());
        const auto utf8 = sgf::core::toUtf8(units);
        if(!utf8) return std::nullopt;
        return std::string(utf8->data(), utf8->size());
#else
        const std::string name(key);
        const char* value = std::getenv(name.c_str());
        if(!value) return std::nullopt;
        return std::string(value);
#endif
    }
}

std::error_code sgf::core::Env::load(const std::filesystem::path& path)
{
    if(path.empty()) return std::make_error_code(std::errc::invalid_argument);

    auto bytes = readFile(path, values_.get_allocator());
    if(!bytes) return bytes.error();

    UnorderedMap<String, String> parsed(values_.get_allocator());
    const std::string_view content = bytes->empty()
        ? std::string_view {}
        : std::string_view { reinterpret_cast<const char*>(bytes->data()), bytes->size() };
    if(const auto err = parseEnv(content, parsed)) return err;

    values_.swap(parsed);
    return {};
}

sgf::core::String sgf::core::Env::get(std::string_view key) const
{
    if(const auto process = processEnvironment(key))
    {
        return String(process->data(), process->size(), get_allocator());
    }

    const String stored_key(key, get_allocator());
    if(const auto found = values_.find(stored_key); found != values_.end())
    {
        return found->second;
    }

    return String(get_allocator());
}
