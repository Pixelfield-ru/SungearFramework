//
// Created by stuka on 31.10.2026.
//

#pragma once

#include "sgf/serialization/spec/serializer_spec.h"
#include "sgf/serialization/serializer.h"

#include <string>

namespace sgf::serialization
{
    template<typename CharT, typename AllocatorT>
    struct SerializerSpec<std::basic_string<CharT, std::char_traits<CharT>, AllocatorT>> final
    {
        using Value = std::basic_string<CharT, std::char_traits<CharT>, AllocatorT>;

        template<typename... ArgsT>
        static std::error_code serialize(Serializer& serializer, const Value& value, ArgsT&&... args)
        {
            return serializer.writer()->writeString(value);
        }

        template<typename... ArgsT>
        static std::error_code deserialize(Serializer& serializer, Value& value, ArgsT&&... args)
        {
            return serializer.reader()->readString(value);
        }
    };

    template<typename IntegralT>
    requires(std::is_integral_v<IntegralT>)
    struct SerializerSpec<IntegralT> final
    {
        template<typename... ArgsT>
        static std::error_code serialize(Serializer& serializer, IntegralT value, ArgsT&&... args)
        {
            auto* writer = serializer.writer();

            if(std::is_signed_v<IntegralT>)
            {
                return writer->writeInt(value);
            }

            return writer->writeUint(value);
        }

        template<typename... ArgsT>
        static std::error_code deserialize(Serializer& serializer, IntegralT& value, ArgsT&&... args)
        {
            auto* reader = serializer.reader();

            if(std::is_signed_v<IntegralT>)
            {
                return reader->readInt(value);
            }

            return reader->readUint(value);
        }
    };

    template<typename FloatingT>
    requires(std::is_floating_point_v<FloatingT>)
    struct SerializerSpec<FloatingT> final
    {
        template<typename... ArgsT>
        static std::error_code serialize(Serializer& serializer, FloatingT value, ArgsT&&... args)
        {
            return serializer.writer()->writeFloat(value);
        }

        template<typename... ArgsT>
        static std::error_code deserialize(Serializer& serializer, FloatingT& value, ArgsT&&... args)
        {
            return serializer.reader()->readFloat(value);
        }
    };

    template<>
    struct SerializerSpec<bool> final
    {
        template<typename... ArgsT>
        static std::error_code serialize(Serializer& serializer, bool value, ArgsT&&... args)
        {
            return serializer.writer()->writeBool(value);
        }

        template<typename... ArgsT>
        static std::error_code deserialize(Serializer& serializer, bool& value, ArgsT&&... args)
        {
            return serializer.reader()->readBool(value);
        }
    };

    template<typename T, typename AllocatorT>
    struct SerializerSpec<std::vector<T, AllocatorT>> final
    {
        using Value = std::vector<T, AllocatorT>;

        template<typename... ArgsT>
        static std::error_code serialize(Serializer& serializer, const Value& value, ArgsT&&... args)
        {
            auto* writer = serializer.writer();

            if(const auto err = writer->beginArray()) return err;

            for(const auto& e : value)
            {
                if(const auto err = serializer.serialize(e, std::forward<ArgsT>(args)...))
                {
                    return err;
                }
            }

            if(const auto err = writer->endArray()) return err;

            return {};
        }

        template<typename... ArgsT>
        static std::error_code deserialize(Serializer& serializer, Value& value, ArgsT&&... args)
        {
            auto* reader = serializer.reader();

            // todo: continue
            // if(const auto err = reader->) return err;
        }
    };
}
