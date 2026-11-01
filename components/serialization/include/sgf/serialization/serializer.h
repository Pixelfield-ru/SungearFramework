//
// Created by stuka on 31.10.2026.
//

#pragma once

#include "format/reader.h"
#include "format/writer.h"

#include "connection.h"
#include "spec/serializer_spec.h"

namespace sgf::serialization
{
    struct Serializer final
    {
        template<typename, typename, typename...>
        friend struct TypesConnection;

        Serializer(IWriter* writer, IReader* reader) noexcept :
            writer_(writer),
            reader_(reader)
        { }
        Serializer(const Serializer&) = default;
        Serializer(Serializer&&) = default;

        template<typename T, typename... ArgsT>
        [[nodiscard]] std::error_code serialize(const T& value, ArgsT&&... args)
        {
            if constexpr(std::is_polymorphic_v<T>)
            {
                std::expected<bool, std::error_code> result;
                impl::Connections<T>::template iterate<ArgsT...>([&](IConnection<ArgsT...>* connection) {
                    // if an error has occurred or object was already serialized
                    if(!result || *result) return;

                    result = connection->serialize(*this, value, std::forward<ArgsT>(args)...);
                });

                if(!result) return result.error();
            }

            return serializeWithBaseTypes(value, std::forward<ArgsT>(args)...);
        }

        template<typename T, typename... ArgsT>
        [[nodiscard]] std::error_code deserialize(T& value, ArgsT&&... args)
        {
            return {};
        }

        [[nodiscard]] IWriter* writer() const noexcept
        {
            return writer_;
        }

        [[nodiscard]] IReader* reader() const noexcept
        {
            return reader_;
        }

        Serializer& operator=(const Serializer&) = default;
        Serializer& operator=(Serializer&&) = default;

    private:
        IWriter* writer_ {};
        IReader* reader_ {};

        template<typename T, typename... ArgsT>
        [[nodiscard]] std::error_code serializeWithBaseTypes(const T& value, ArgsT&&... args) noexcept
        {
            using base_types = decltype(impl::collectBaseTypes<T>());

            std::error_code err {};
            core::forTypes(base_types{}, [&](auto wrapper) {
                if(err) return;

                using type = decltype(wrapper)::type;

                err = SerializerSpec<type>::serialize(*this, value, std::forward<ArgsT>(args)...);
            });

            return err;
        }
    };
}
