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
        std::expected<bool, std::error_code> serialize(Serializer& serializer, const void* object, ArgsT&&... args) const noexcept override
        {
            const auto invoke_connections = [&]<typename... ArgsT0>(ArgsT0&&... args0) {
                std::expected<bool, std::error_code> result;

                impl::Connections<Derived>::template iterate<ArgsT0...>([&](IConnection<ArgsT0...>* connection) {
                    // if error or was serialized or the connection is current
                    if(!result || *result || this == connection) return;

                    result = connection->serialize(serializer, object, std::forward<ArgsT0>(args0)...);
                });

                return result;
            };

            // trying to serialize with args
            auto derived_serialization_res = invoke_connections(std::forward<ArgsT>(args)...);
            if(derived_serialization_res && *derived_serialization_res) return true;

            // trying to serialize without args
            derived_serialization_res = invoke_connections();
            if(derived_serialization_res && *derived_serialization_res) return true;

            // otherwise, we decide that the current type is the actual type

            const auto* typed_object = static_cast<const CurrentType*>(object);
            const auto* derived_object = dynamic_cast<const Derived*>(typed_object);

            if(!derived_object) return false;

            if(const auto err = serializer.serializeWithBaseTypes(*derived_object, std::forward<ArgsT>(args)...))
            {
                return std::unexpected(err);
            }

            return true;
        }

        std::expected<bool, std::error_code> deserialize(Serializer& serializer, void* object, ArgsT&&... args) const noexcept override
        {
            return true;
        }
    };
}
