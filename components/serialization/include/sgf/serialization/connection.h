//
// Created by stuka on 31.10.2026.
//

#pragma once

#include <cstdint>
#include <expected>
#include <system_error>

#include "sgf/core/memory.h"
#include "sgf/core/utility/type_traits.h"

namespace sgf::serialization
{
    struct Serializer;

    /// Used for derived types.
    template<typename... ArgsT>
    struct IConnection
    {
        virtual ~IConnection() = default;

        virtual std::expected<bool, std::error_code> serialize(Serializer& serializer, const void* object, ArgsT&&... args) const noexcept = 0;
        virtual std::expected<bool, std::error_code> deserialize(Serializer& serializer, void* object, ArgsT&&... args) const noexcept = 0;
    };

    namespace impl
    {
        /// Be careful with reference that this function returns. It can be invalidated if new serializer will be added.\n
        /// DO NOT STORE REFERENCE.
        core::Vector<std::byte*>& connections(size_t type_hash) noexcept;

        template<typename T, typename... ArgsT>
        size_t storageTypeHash() noexcept
        {
            static size_t hash = std::hash<const char*>{}(GENERATOR_PRETTY_FUNCTION);
            return hash;
        }

        template<typename T>
        struct Connections
        {
            template<typename... ArgsT, typename Fn>
            static void iterate(Fn&& func) noexcept
            {
                static size_t type_hash = storageTypeHash<T, ArgsT...>();

                const auto type_connections = connections(type_hash);
                for(auto* connection : type_connections)
                {
                    std::forward<Fn>(func)(reinterpret_cast<IConnection<ArgsT...>*>(connection));
                }
            }

            template<typename... ArgsT>
            static void add(IConnection<ArgsT...>* connection) noexcept
            {
                static size_t type_hash = storageTypeHash<T, ArgsT...>();
                connections(type_hash).push_back(reinterpret_cast<std::byte*>(connection));
            }
        };
    }
}
