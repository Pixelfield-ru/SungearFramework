//
// Created by stuka on 01.11.2026.
//

#include "sgf/serialization/connection.h"

sgf::core::Vector<std::byte*>& sgf::serialization::impl::connections(size_t type_hash) noexcept
{
    static core::UnorderedMap<size_t, core::Vector<std::byte*>> serializers;
    return serializers[type_hash];
}
