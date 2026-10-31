//
// Created by stuka on 31.10.2026.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <system_error>

namespace sgf::serialization
{
    enum class FormatNode : std::uint8_t
    {
        kInvalid,
        kNull,
        kBool,
        kInt,
        kUint,
        kFloat,
        kString,
        kObject,
        kArray
    };

    struct IReader
    {
        virtual ~IReader() = default;

        [[nodiscard]] virtual FormatNode kind() const = 0;
        [[nodiscard]] virtual std::size_t size() const = 0;

        [[nodiscard]] virtual IReader* field(std::string_view key) = 0;
        [[nodiscard]] virtual IReader* at(std::size_t index) = 0;

        virtual std::error_code readBool(bool& value) const = 0;
        virtual std::error_code readInt(std::int64_t& value) const = 0;
        virtual std::error_code readUint(std::uint64_t& value) const = 0;
        virtual std::error_code readFloat(double& value) const = 0;
        virtual std::error_code readString(std::string_view& value) const = 0;
    };
}
