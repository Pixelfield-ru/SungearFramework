//
// Created by stuka on 12.10.2026.
//

#include "sgf/core/utility/encoding.h"

#include <string_view>

namespace
{
    void appendUtf8(sgf::core::String& out, char32_t codepoint)
    {
        if(codepoint <= 0x7F)
        {
            out.push_back(static_cast<char>(codepoint));
            return;
        }

        if(codepoint <= 0x7FF)
        {
            out.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
            return;
        }

        if(codepoint >= 0xD800 && codepoint <= 0xDFFF)
            return;

        if(codepoint <= 0xFFFF)
        {
            out.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
            out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
            return;
        }

        if(codepoint > 0x10FFFF)
            return;

        out.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
        out.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }

    void appendUtf16(sgf::core::BasicString<char16_t>& out, char32_t codepoint)
    {
        if(codepoint <= 0xFFFF)
        {
            out.push_back(static_cast<char16_t>(codepoint));
            return;
        }

        const char32_t shifted = codepoint - 0x10000;
        out.push_back(static_cast<char16_t>(0xD800 | (shifted >> 10)));
        out.push_back(static_cast<char16_t>(0xDC00 | (shifted & 0x3FF)));
    }

    std::error_code readUtf8(std::string_view& text, char32_t& codepoint)
    {
        if(text.empty()) return std::make_error_code(std::errc::invalid_argument);

        const auto lead = static_cast<unsigned char>(text.front());
        std::size_t width = 1;
        char32_t value = lead;

        if(lead <= 0x7F)
        {
            width = 1;
            value = lead;
        }
        else if((lead & 0xE0) == 0xC0)
        {
            width = 2;
            value = lead & 0x1F;
        }
        else if((lead & 0xF0) == 0xE0)
        {
            width = 3;
            value = lead & 0x0F;
        }
        else if((lead & 0xF8) == 0xF0)
        {
            width = 4;
            value = lead & 0x07;
        }
        else
        {
            return std::make_error_code(std::errc::invalid_argument);
        }

        if(text.size() < width) return std::make_error_code(std::errc::invalid_argument);

        for(std::size_t index = 1; index < width; ++index)
        {
            const auto continuation = static_cast<unsigned char>(text[index]);
            if((continuation & 0xC0) != 0x80) return std::make_error_code(std::errc::invalid_argument);
            value = (value << 6) | (continuation & 0x3F);
        }

        const bool overlong = (width == 2 && value < 0x80)
                           || (width == 3 && value < 0x800)
                           || (width == 4 && value < 0x10000);
        if(overlong || value > 0x10FFFF || (value >= 0xD800 && value <= 0xDFFF))
        {
            return std::make_error_code(std::errc::invalid_argument);
        }

        text.remove_prefix(width);
        codepoint = value;
        return {};
    }

    std::error_code readUtf16(std::u16string_view& text, char32_t& codepoint)
    {
        if(text.empty()) return std::make_error_code(std::errc::invalid_argument);

        const char16_t unit = text.front();
        text.remove_prefix(1);

        if(unit < 0xD800 || unit > 0xDFFF)
        {
            codepoint = unit;
            return {};
        }

        if(unit > 0xDBFF || text.empty()) return std::make_error_code(std::errc::invalid_argument);

        const char16_t low = text.front();
        if(low < 0xDC00 || low > 0xDFFF) return std::make_error_code(std::errc::invalid_argument);

        text.remove_prefix(1);
        codepoint = 0x10000 + ((static_cast<char32_t>(unit - 0xD800) << 10) | (low - 0xDC00));
        return {};
    }
}

sgf::core::String sgf::core::toUtf8(char32_t codepoint, const std::pmr::polymorphic_allocator<>& alloc)
{
    String out(alloc);
    appendUtf8(out, codepoint);
    return out;
}

sgf::core::String sgf::core::toUtf8(std::span<const char32_t> codepoints, const std::pmr::polymorphic_allocator<>& alloc)
{
    String out(alloc);
    out.reserve(codepoints.size() * 4);

    for(char32_t codepoint : codepoints)
        appendUtf8(out, codepoint);

    return out;
}

std::expected<sgf::core::String, std::error_code> sgf::core::toUtf8(
    std::u16string_view text, const std::pmr::polymorphic_allocator<>& alloc)
{
    String out(alloc);
    out.reserve(text.size());

    while(!text.empty())
    {
        char32_t codepoint = 0;
        if(const auto err = readUtf16(text, codepoint)) return std::unexpected(err);
        appendUtf8(out, codepoint);
    }

    return out;
}

std::expected<sgf::core::BasicString<char16_t>, std::error_code> sgf::core::toUtf16(
    std::string_view text, const std::pmr::polymorphic_allocator<>& alloc)
{
    BasicString<char16_t> out(alloc);
    out.reserve(text.size());

    while(!text.empty())
    {
        char32_t codepoint = 0;
        if(const auto err = readUtf8(text, codepoint)) return std::unexpected(err);
        appendUtf16(out, codepoint);
    }

    return out;
}

sgf::core::String sgf::core::pathToUtf8(
    const std::filesystem::path& path, const std::pmr::polymorphic_allocator<>& alloc) noexcept
{
    const auto u8 = path.string<char8_t>(std::pmr::polymorphic_allocator<char8_t> { alloc });
    if(u8.empty()) return String(alloc);
    return String(reinterpret_cast<const char*>(u8.data()), u8.size(), alloc);
}
