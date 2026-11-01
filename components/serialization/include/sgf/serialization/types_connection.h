//
// Created by stuka on 01.11.2026.
//

#pragma once

#include "connection.h"

namespace sgf::serialization
{
    template<typename CurrentType, typename Derived, typename... ArgsT>
    struct TypesConnection final : IConnection<ArgsT...>
    {
        std::expected<bool, std::error_code> serialize(Serializer& serializer, const std::byte* object, ArgsT&&... args) const noexcept override
        {
            const auto* typed_object = reinterpret_cast<const CurrentType*>(object);
            const auto* derived_object = dynamic_cast<const Derived*>(typed_object);

            if(!derived_object) return false;

            const auto invoke_connections = [&]<typename... ArgsT0>(ArgsT0&&... args0) {
                std::expected<bool, std::error_code> result;

                impl::Connections<Derived>::template iterate<ArgsT0...>([&](IConnection<ArgsT0...>* connection) {
                    // if error or was serialized or the connection is current
                    if(!result || *result || this == connection) return;

                    result = connection->serialize(serializer, object, std::forward<ArgsT0>(args0)...);
                });

                return result;
            };

            // check that serialization can proceed further
            auto derived_serialization_res = invoke_connections(std::forward<ArgsT>(args)...);
            if(derived_serialization_res && *derived_serialization_res) return true;

            // otherwise, we decide that the current Derived type is the actual type

            // write actual type of object
            if constexpr(requires { SerializerSpec<Derived>::type_name; })
            {
                if(const auto err = serializer.writer()->key(impl::key_type_name))
                    return std::unexpected(err);

                if(const auto err = serializer.writer()->writeString(SerializerSpec<Derived>::type_name))
                    return std::unexpected(err);
            }
            else
            {
                static_assert(core::always_false_v<Derived>, "This type is polymorphic, use SGF_SERIALIZER_TYPE_NAME(...) in your SerializerSpec!");
            }

            // serialize all bases of Derived and Derived itself
            if(const auto err = serializer.serializeWithBaseTypes(*derived_object, std::forward<ArgsT>(args)...))
            {
                return std::unexpected(err);
            }

            return true;
        }

        std::expected<OwnedObject, std::error_code> deserialize(Serializer& serializer, ArgsT&&... args) const noexcept override
        {
            const auto invoke_connections = [&]<typename... ArgsT0>(ArgsT0&&... args0) {
                std::expected<OwnedObject, std::error_code> result;

                impl::Connections<Derived>::template iterate<ArgsT0...>([&](IConnection<ArgsT0...>* connection) {
                    // if error or was serialized or the connection is current
                    if(!result || *result || this == connection) return;

                    result = connection->deserialize(serializer, std::forward<ArgsT0>(args0)...);
                });

                return result;
            };

            // check that deserialization can proceed further
            auto derived_deserialization_res = invoke_connections(std::forward<ArgsT>(args)...);
            if(derived_deserialization_res && *derived_deserialization_res) return std::move(*derived_deserialization_res);

            // otherwise, we decide that the Derived type is the actual type

            // write actual type of object
            if constexpr(!requires { SerializerSpec<Derived>::type_name; })
            {
                static_assert(core::always_false_v<Derived>, "This type is polymorphic, use SGF_SERIALIZER_TYPE_NAME(...) in your SerializerSpec!");
            }

            if(const auto err = serializer.reader()->field(impl::key_type_name))
                return std::unexpected(err);

            std::string_view type_name;
            if(const auto err = serializer.reader()->readString(type_name))
                return std::unexpected(err);
            if(const auto err = serializer.reader()->leave())
                return std::unexpected(err);

            // not derived type
            if(type_name != SerializerSpec<Derived>::type_name) return OwnedObject {};

            auto object = core::makeUnique<Derived>(serializer.get_allocator());

            // serialize all bases of Derived and Derived itself
            if(const auto err = serializer.deserializeWithBaseTypes(*object, std::forward<ArgsT>(args)...))
            {
                return std::unexpected(err);
            }

            const auto deleter = object.get_deleter();
            void* pointer = object.release();

            return OwnedObject(pointer, deleter.resource, deleter.destroy);
        }
    };
}
