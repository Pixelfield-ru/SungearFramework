//
// Created by stuka on 31.10.2026.
//

#pragma once

#include "sgf/core/utility/type_traits.h"

#include "sgf/serialization/format/writer.h"
#include "sgf/serialization/format/reader.h"

#include <system_error>

namespace sgf::serialization
{
    struct Serializer;

    template<typename T>
    struct SerializerSpec final
    {
        static_assert(core::always_false_v<T>, "Implement SerializerSpec for your type!");

        template<typename... ArgsT>
        static std::error_code serialize(Serializer& serializer, const T& value, ArgsT&&... args)
        {
            return {};
        }

        template<typename... ArgsT>
        static std::error_code deserialize(Serializer& serializer, T& value, ArgsT&&... args)
        {
            return {};
        }
    };
}