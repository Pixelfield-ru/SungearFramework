//
// Created by stuka on 12.10.2026.
//

#include "sgf/core/utility/encoding.h"

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

sgf::core::String sgf::core::pathToUtf8(const std::filesystem::path& path) noexcept
{
    const auto u8 = path.u8string();
    return { u8.begin(), u8.end() };
}
