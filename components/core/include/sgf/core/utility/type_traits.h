//
// Created by stuka on 14.10.2026.
//

#pragma once

#include <cstdint>
#include <tuple>
#include <type_traits>

#if defined _MSC_VER
#   define GENERATOR_PRETTY_FUNCTION __FUNCSIG__
#elif defined __clang__ || (defined __GNUC__)
#   define GENERATOR_PRETTY_FUNCTION __PRETTY_FUNCTION__
#endif

namespace sgf::core
{
    namespace impl
    {
        std::uint32_t nextIndex() noexcept;
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
    struct type_wrapper final
    {
        using type = T;
    };

    template<typename>
    struct always_false : std::false_type { };

    template<typename T>
    static constexpr bool always_false_v = always_false<T>::value;

    template<std::size_t Idx, typename... Types>
    class get_type
    {
        static_assert(Idx < sizeof...(Types), "Index out of bounds.");

        template<size_t I, size_t N, typename... Rest>
        struct impl;

        template<size_t I, size_t N, typename T, typename... Rest>
        struct impl<I, N, T, Rest...>
        {
            using type = impl<I + 1, N, Rest...>::type;
        };

        template<size_t N, typename T, typename... Rest>
        struct impl<N, N, T, Rest...>
        {
            using type = T;
        };
    public:
        using type = impl<0, Idx, Types...>::type;
    };

    template<std::size_t Index, typename... Types>
    using get_type_t = get_type<Index, Types...>::type;

    template<typename T, typename... Types>
    class find_type
    {
        template<std::int64_t CurrentIndex, typename... Rest>
        struct impl
        {
            static constexpr std::int64_t index = -1;
        };

        template<std::int64_t CurrentIndex, typename Head, typename... Rest>
        struct impl<CurrentIndex, Head, Rest...>
        {
            static constexpr std::int64_t index = impl<CurrentIndex + 1, Rest...>::index;
        };

        template<std::int64_t CurrentIndex, typename... Rest>
        struct impl<CurrentIndex, T, Rest...>
        {
            static constexpr std::int64_t index = CurrentIndex;
        };

    public:
        static constexpr std::int64_t index = impl<0, Types...>::index;
    };

    template<typename... Types>
    struct types_container
    {
        template<size_t Index>
        using get = get_type_t<Index, Types...>;

        template<typename T>
        static constexpr bool contains = find_type<T, Types...>::index != -1;

        static constexpr size_t count = sizeof...(Types);

        template<typename... Types1>
        constexpr auto operator+(types_container<Types1...>) const noexcept
        {
            return types_container<Types..., Types1...>{};
        }

        template<typename Type1>
        constexpr auto operator+(type_wrapper<Type1>) const noexcept
        {
            return types_container<Types..., Type1>{};
        }

        [[nodiscard]] constexpr auto flip() const noexcept
        {
            return []<size_t... Indices>(std::index_sequence<Indices...>) {
                return types_container<get<count - 1 - Indices>...>{};
            }(std::make_index_sequence<count>{});
        }

        [[nodiscard]] constexpr auto unify() const noexcept
        {
            return typename unify_impl<types_container<>, Types...>::type{};
        }

    private:
        template<typename Accumulated, typename... Rest>
        struct unify_impl
        {
            using type = Accumulated;
        };

        template<typename... Kept, typename Head, typename... Tail>
        struct unify_impl<types_container<Kept...>, Head, Tail...>
        {
            using next = std::conditional_t<
                types_container<Kept...>::template contains<Head>,
                types_container<Kept...>,
                types_container<Kept..., Head>>;

            using type = typename unify_impl<next, Tail...>::type;
        };
    };

    template<typename... Types, typename Fn>
    void forTypes(Fn&& func) noexcept
    {
        (std::forward<Fn>(func)(type_wrapper<Types>{}), ...);
    }

    template<typename... Types, typename Fn>
    void forTypes(types_container<Types...>, Fn&& func) noexcept
    {
        (std::forward<Fn>(func)(type_wrapper<Types>{}), ...);
    }

    template<typename... Types>
    struct base_types
    {
        using base = types_container<Types...>;
    };

    template<typename... Types>
    struct derived_types
    {
        using derived = types_container<Types...>;
    };
}
