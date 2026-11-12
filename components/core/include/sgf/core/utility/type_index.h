//
// Created by stuka on 12.11.2026.
//

#pragma once

#include <cstdint>
#include <format>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>

#include "assert.h"
#include "platform.h"

namespace sgf::core
{
    namespace impl
    {
        std::uint32_t nextIndex() noexcept;

        struct StableTypeRecord
        {
            std::string name;
            std::uint32_t type_index {};
        };

        std::unordered_map<std::uint32_t, StableTypeRecord>& stableTypes();

        inline std::shared_mutex& stableTypesMutex()
        {
            static std::shared_mutex mutex;
            return mutex;
        }
    }

    /**
     * @return Sequenced index for type.
     */
    template<typename>
    std::uint32_t typeIndex() noexcept
    {
        static const auto index = impl::nextIndex();

        return index;
    }

    template<typename T>
    consteval std::string_view typeName() noexcept
    {
        if constexpr(std::is_same_v<T, std::int64_t>) return "int64";
        if constexpr(std::is_same_v<T, std::uint64_t>) return "uint64";
        if constexpr(std::is_same_v<T, std::int16_t>) return "int16";
        if constexpr(std::is_same_v<T, std::uint16_t>) return "uint16";
        if constexpr(std::is_same_v<T, std::int32_t>) return "int32";
        if constexpr(std::is_same_v<T, std::uint32_t>) return "uint32";
        if constexpr(std::is_same_v<T, float>) return "float";

#if SGF_PLATFORM_COMPILER_MSVC
        constexpr std::string_view signature = __FUNCSIG__;
        constexpr auto begin = signature.find("typeName<") + std::string_view("typeName<").size();
        constexpr auto end = signature.rfind(">(");
        constexpr auto raw = signature.substr(begin, end - begin);
        // msvc writes "struct sgf::Mesh", clang and gcc — "sgf::Mesh"
        if constexpr(raw.starts_with("struct ")) return raw.substr(sizeof("struct ") - 1);
        if constexpr(raw.starts_with("class ")) return raw.substr(sizeof("class ") - 1);
        if constexpr(raw.starts_with("enum ")) return raw.substr(sizeof("enum ") - 1);
        if constexpr(raw.starts_with("union ")) return raw.substr(sizeof("union ") - 1);

        return raw;
#else
        constexpr std::string_view signature = __PRETTY_FUNCTION__;
        // clang: "typeName() [T = sgf::Mesh]", gcc: "typeName() [with T = sgf::Mesh]"
#if SGF_PLATFORM_COMPILER_GCC
        constexpr auto marker = std::string_view("[with T = ");
#else
        constexpr auto marker = std::string_view("[T = ");
#endif
        constexpr auto begin = signature.rfind(marker) + marker.size();
        constexpr auto bracket = signature.find(']', begin);
        constexpr auto semicolon = signature.find(';', begin);
        constexpr auto end = semicolon < bracket ? semicolon : bracket;
        return signature.substr(begin, end - begin);
#endif
    }

    constexpr std::uint32_t stableHash(std::string_view name) noexcept
    {
        std::uint32_t hash = 2166136261u;
        for(const char byte : name)
        {
            hash ^= static_cast<std::uint8_t>(byte);
            hash *= 16777619u;
        }
        return hash;
    }

    template<typename T>
    consteval std::uint32_t stableTypeHash() noexcept
    {
        return stableHash(typeName<T>());
    }

    /**
     * Records the stable hash of T and its process-local typeIndex.
     * Registering the same name again returns the hash already stored.
     * In debug, a zero hash or a collision with a different name asserts and the message names the types.
     * @return Stable hash of T.
     */
    template<typename T>
    std::uint32_t registerStableType()
    {
        constexpr auto name = typeName<T>();
        constexpr auto id = stableTypeHash<T>();

        SGF_ASSERT(id != 0, std::format("Stable type hash is zero: {}", name).c_str());

        std::unique_lock lock(impl::stableTypesMutex());

        auto& types = impl::stableTypes();
        if(const auto it = types.find(id); it != types.end())
        {
            SGF_ASSERT(it->second.name == name, std::format("Stable type hash collision: {} and {}", it->second.name, name).c_str());
            return id;
        }

        types.emplace(id, impl::StableTypeRecord { std::string(name), typeIndex<T>() });

        return id;
    }

    /**
     * Process-local typeIndex for a stable hash registered in this run.
     * In debug, an unknown hash asserts and the message includes the hash.
     * @return Process-local typeIndex.
     */
    [[nodiscard]] std::uint32_t typeIndexFromStableHash(std::uint32_t stable_hash);
}
