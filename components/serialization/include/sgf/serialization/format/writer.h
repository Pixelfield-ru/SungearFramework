//
// Created by stuka on 31.10.2026.
//

#pragma once

#include <system_error>
#include <string_view>
#include <cstdint>

namespace sgf::serialization
{
    struct WriterSettings
    {
        bool pretty { false };
        std::uint32_t spaces { 4 };
    };

    struct IWriter
    {
        virtual ~IWriter() = default;

        virtual std::error_code setSettings(const WriterSettings& settings) = 0;

        virtual std::error_code beginObject() = 0;
        virtual std::error_code endObject() = 0;
        virtual std::error_code beginArray() = 0;
        virtual std::error_code endArray() = 0;

        virtual std::error_code key(std::string_view name) = 0;

        virtual std::error_code writeNull() = 0;
        virtual std::error_code writeBool(bool value) = 0;
        virtual std::error_code writeInt(std::int64_t value) = 0;
        virtual std::error_code writeUint(std::uint64_t value) = 0;
        virtual std::error_code writeFloat(double value) = 0;
        virtual std::error_code writeString(std::string_view value) = 0;
    };
}