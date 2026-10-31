//
// Created by stuka on 31.10.2026.
//

#pragma once

#include "format/reader.h"
#include "format/writer.h"
#include "sgf/core/utility/encoding.h"

namespace sgf::serialization
{
    struct Serializer final
    {
        Serializer(IWriter* writer, IReader* reader) noexcept :
            writer_(writer),
            reader_(reader)
        { }
        Serializer(const Serializer&) = default;
        Serializer(Serializer&&) = default;

        template<typename T, typename... ArgsT>
        std::error_code serialize(const T& value, ArgsT&&... args)
        {
            return {};
        }

        template<typename T, typename... ArgsT>
        std::error_code deserialize(T& value, ArgsT&&... args)
        {
            return {};
        }

        IWriter* writer() const noexcept
        {
            return writer_;
        }

        IReader* reader() const noexcept
        {
            return reader_;
        }

        Serializer& operator=(const Serializer&) = default;
        Serializer& operator=(Serializer&&) = default;

    private:
        IWriter* writer_ {};
        IReader* reader_ {};
    };
}
