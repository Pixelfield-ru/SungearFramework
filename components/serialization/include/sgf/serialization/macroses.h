//
// Created by stuka on 01.11.2026.
//

#pragma once

#include "sgf/core/utility/macroses.h"

namespace sgf::serialization
{
    template<typename>
    struct SerializerSpec;
}

#define SGF_SERIALIZER_ADD_CONNECTION(Base, Derived, ...) \
auto SGF_CONCAT(_SG_DECLARE_DESERIALIZER_SPEC_, __LINE__) = []() { \
    static sgf::serialization::TypesConnection<Base, Derived, ##__VA_ARGS__> connection; \
    sgf::serialization::impl::Connections<Base>::add<__VA_ARGS__>(&connection); \
    return true; \
}();

#define SGF_SERIALIZER_TYPE_NAME(name) \
    static constexpr std::string_view type_name { #name };

#define SGF_SERIALIZER_AS_FRIEND() \
    template<typename> \
    friend struct sgf::serialization::SerializerSpec;