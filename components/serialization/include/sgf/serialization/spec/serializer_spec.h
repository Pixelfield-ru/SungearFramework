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

    /**
     * Serialization specialization for type. Inherit core::base_types to specify the base types.
     * @tparam T Type.
     */
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

    namespace impl
    {
        template<typename T>
        auto collectBaseTypes() noexcept
        {
            if constexpr (requires { typename SerializerSpec<T>::base; })
            {
                using bases = typename SerializerSpec<T>::base;

                return []<std::size_t... Indices>(std::index_sequence<Indices...>)
                {
                    if constexpr (sizeof...(Indices) == 0)
                    {
                        return core::types_container<>{};
                    }
                    else
                    {
                        return ((collectBaseTypes<typename bases::template get<Indices>>()
                                 + core::type_wrapper<typename bases::template get<Indices>>{}) + ...);
                    }
                }(std::make_index_sequence<bases::count>{}).unify();
            }
            else return core::types_container<>{};
        }
    }
}