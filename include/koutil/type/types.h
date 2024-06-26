#ifndef KOUTIL_UTIL_TYPES_H
#define KOUTIL_UTIL_TYPES_H

#include <array>
#include <cassert>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace koutil::type {

/**
 * @brief A structure representing a variadic list of types.
 *
 * @tparam T The types in the list.
 */
template <typename... T> struct types {
    static constexpr std::size_t size = sizeof...(T);
};

template <typename T> struct are_types : std::false_type { };

template <typename... Types> struct are_types<types<Types...>> : std::true_type { };

template <typename T> inline constexpr bool are_types_v = are_types<T>::value;

template <typename T>
concept types_concept = are_types_v<T>;

/**
 * @brief Concatenates two types lists.
 *
 * @tparam T The first types list.
 * @tparam U The second types list.
 */
template <types_concept T, types_concept U> struct types_cat;

/**
 * @brief Specialization for concatenating two `types` lists.
 *
 * @tparam T The first types list.
 * @tparam U The second types list.
 */
template <typename... T, typename... U> struct types_cat<types<T...>, types<U...>> {
    using type = types<T..., U...>;
};

/**
 * @brief Alias for the concatenated types list.
 *
 * @tparam T The first types list.
 * @tparam U The second types list.
 */
template <types_concept T, types_concept U> using types_cat_t = types_cat<T, U>::type;

/**
 * @brief Concept for a type that provides a `transform` template.
 */
template <typename T>
concept types_transform = requires() {
    typename T::template transform<int>;
    typename T::template transform<int, int>;
    typename T::template transform<int, int, int, int, int>;
};

/**
 * @brief Concept for a type that provides a `container` template.
 */
template <typename T>
concept types_container = requires() { typename T::template container<int>; };

/**
 * @brief A namespace containing transform templates for types lists.
 */
struct types_transforms {
    /**
     * @brief Provides a `std::tuple` transform.
     */
    struct tuple {
        template <typename... Types> using transform = std::tuple<Types...>;
    };

    /**
     * @brief Provides a `std::variant` transform.
     */
    struct variant {
        template <typename... Types> using transform = std::variant<Types...>;
    };

    /**
     * @brief Provides a `T&` transform.
     */
    struct reference {
        template <typename... Types> using transform = types<Types&...>;
    };

    /**
     * @brief Provides a `const T` transform.
     */
    struct constant {
        template <typename... Types> using transform = types<const Types...>;
    };

    /*
     * @brief Provides a `const T&` transform.
     */
    struct constant_reference {
        template <typename... Types> using transform = types<const Types&...>;
    };

    /**
     * @brief Provides a `std::vector<T>::reference` transform.
     */
    struct vector_reference {
        template <typename... Types> using transform = types<typename std::vector<Types>::reference...>;
    };

    /*
     * @brief Provides a `std::vector<T>::const_reference` transform.
     */
    struct vector_const_reference {
        template <typename... Types> using transform = types<typename std::vector<Types>::const_reference...>;
    };
};

/**
 * @brief A namespace containing container templates for types lists.
 */
struct types_containers {
    /**
     * @brief Provides a `std::vector` container.
     */
    struct vector {
        template <typename T> using container = std::vector<T>;
    };
};

/**
 * @brief A template structure to count occurrences of a specific type in a types list.
 *
 * @tparam Types The types list.
 */
template <types_concept Types> struct types_count;

/**
 * @brief Specialization to count occurrences of a specific type in a `types` list.
 *
 * @tparam Types The types in the list.
 */
template <typename... Types> struct types_count<types<Types...>> {
    /**
     * @brief Computes the number of occurrences of type `T` in the types list.
     *
     * @tparam T The type to count.
     * @return The number of occurrences of `T` in the types list.
     */
    template <typename T> static constexpr std::size_t value = 0 + (std::is_same_v<T, Types> + ...);
};

namespace detail {
    /**
     * @brief Helper to check if a type is contained in a list of types.
     *
     * @tparam T The type to check.
     * @tparam Other The list of other types.
     * @return True if `T` is contained in `Other`, false otherwise.
     */
    template <typename T, typename... Other> consteval bool contains_type() {
        return (std::is_same_v<T, Other> || ...);
    }

    /**
     * @brief Implementation for creating a unique types list.
     *
     * @tparam Types The types in the list.
     */
    template <typename... Types> struct unique_types_impl;

    /**
     * @brief Specialization for a single type.
     *
     * @tparam T The type.
     */
    template <typename T> struct unique_types_impl<T> {
        using type = types<T>;
    };

    /**
     * @brief Specialization for multiple types.
     *
     * @tparam T The first type.
     * @tparam Other The other types.
     */
    template <typename T, typename... Other> struct unique_types_impl<T, Other...> {
        using other_types = unique_types_impl<Other...>::type;
        using type = std::conditional_t<contains_type<T, Other...>(), other_types, types_cat_t<types<T>, other_types>>;
    };

    /**
     * @brief Transforms a types list to a containers list.
     *
     * @tparam T The types list.
     * @tparam Container The container type.
     */
    template <types_concept T, types_container Container> struct types_to_containers;

    /**
     * @brief Specialization for transforming a types list to a containers list.
     *
     * @tparam Types The types in the list.
     * @tparam Container The container type.
     */
    template <typename... Types, types_container Container> struct types_to_containers<types<Types...>, Container> {
        using type = types<typename Container::template container<Types>...>;
    };

    /**
     * @brief Implementation for transforming a types list using a transform template.
     *
     * @tparam T The types list.
     * @tparam Transform The transform template.
     */
    template <types_concept T, types_transform Transform> struct types_transform_impl;

    /**
     * @brief Specialization for transforming a types list using a transform template.
     *
     * @tparam Types The types in the list.
     * @tparam Transform The transform template.
     */
    template <typename... Types, types_transform Transform> struct types_transform_impl<types<Types...>, Transform> {
        using type = Transform::template transform<Types...>;
    };

    /**
     * @brief Helper to get the index of a type in a types list.
     *
     * @tparam T The type to find.
     * @tparam N The number of occurrences to skip.
     * @tparam I The current index.
     * @tparam Type The first type in the list.
     * @tparam Other The other types in the list.
     * @return The index of the `N`-th occurrence of `T` in the types list.
     */
    template <typename T, std::size_t N, std::size_t I, typename Type, typename... Other>
    constexpr std::size_t types_index_of_impl() {
        if constexpr (std::is_same_v<T, Type>) {
            if constexpr (N == 0) {
                return I;
            } else if constexpr (sizeof...(Other) != 0) {
                return types_index_of_impl<T, N - 1, I + 1, Other...>();
            }
        } else if constexpr (sizeof...(Other) != 0) {
            return types_index_of_impl<T, N, I + 1, Other...>();
        }
        assert(false);
    }

    /**
     * @brief Structure to get the index of a type in a types list.
     *
     * @tparam T The type to find.
     * @tparam Types The types list.
     * @tparam N The number of occurrences to skip.
     */
    template <typename T, types_concept Types, std::size_t N> struct types_index_of;

    /**
     * @brief Specialization to get the index of a type in a `types` list.
     *
     * @tparam T The type to find.
     * @tparam Types The types in the list.
     * @tparam N The number of occurrences to skip.
     */
    template <typename T, typename... Types, std::size_t N> struct types_index_of<T, types<Types...>, N> {
        static constexpr std::size_t value = types_index_of_impl<T, N, 0, Types...>();
    };

    /**
     * @brief Implementation for getting a type by index in a types list.
     *
     * @tparam Types The types list.
     * @tparam I The index.
     */
    template <types_concept Types, std::size_t I> struct types_get_impl;

    /**
     * @brief Specialization for getting a type by index in a `types` list.
     *
     * @tparam Types The types in the list.
     * @tparam I The index.
     */
    template <typename... Types, std::size_t I>
        requires(sizeof...(Types) > 0)
    struct types_get_impl<types<Types...>, I> {
        using type = std::tuple_element_t<I, std::tuple<Types...>>;
    };

    template <std::size_t I> struct types_get_impl<types<>, I> {
        using type = void;
    };

    /**
     * @brief Implementation for creating a unique types list from a types list.
     *
     * @tparam Types The types list.
     */
    template <types_concept Types> struct types_unique_impl;

    /**
     * @brief Specialization for creating a unique types list from a `types` list.
     *
     * @tparam Types The types in the list.
     */
    template <typename... Types> struct types_unique_impl<types<Types...>> {
        using type = unique_types_impl<Types...>::type;
    };

    /**
     * @brief Implementation for converting a types list to arrays of unique types.
     *
     * @tparam Types The types list.
     * @tparam Unique The unique types list.
     */
    template <types_concept Types, typename Unique> struct types_to_arrays;

    /**
     * @brief Specialization for converting a types list to arrays of unique types.
     *
     * @tparam Types The types in the list.
     * @tparam Unique The unique types in the list.
     */
    template <typename... Types, typename... Unique> struct types_to_arrays<types<Types...>, types<Unique...>> {
        using type = types<std::array<Unique, types_count<types<Types...>>::template value<Unique>>...>;
    };

    template <typename Search, typename Type, typename... Other> struct types_remove_impl {
        using rest = types_remove_impl<Search, Other...>::type;

        using type = std::conditional_t<std::is_same_v<Type, Search>, rest, types_cat_t<types<Type>, rest>>;
    };

    template <typename Search, typename Type> struct types_remove_impl<Search, Type> {
        using type = std::conditional_t<std::is_same_v<Type, Search>, types<>, types<Type>>;
    };

    template <typename Types, typename Type> struct types_remove;

    template <typename... Types, typename Type>
        requires(sizeof...(Types) > 0)
    struct types_remove<types<Types...>, Type> {
        using type = types_remove_impl<Type, Types...>::type;
    };

    template <typename Type> struct types_remove<types<>, Type> {
        using type = types<>;
    };

    template <std::size_t N, types_concept Types> struct types_view;

    template <std::size_t N, typename Type, typename... Other> struct types_view<N, types<Type, Other...>> {
        using type = types_cat_t<
            types<Type>,
            std::conditional_t<(N > 1), typename types_view<N - 1, types<Other...>>::type, types<>>>;
    };

    template <std::size_t N> struct types_view<N, types<>> {
        using type = types<>;
    };
}

/**
 * @brief Alias for getting a type by index in a types list.
 *
 * @tparam Types The types list.
 * @tparam I The index.
 */
template <types_concept Types, std::size_t I> using types_get_t = detail::types_get_impl<Types, I>::type;

/**
 * @brief Alias for creating a unique types list from a list of types.
 *
 * @tparam Types The types in the list.
 */
template <typename... Types> using unique_types_t = detail::unique_types_impl<Types...>::type;

/**
 * @brief Alias for creating a unique types list from a `types` list.
 *
 * @tparam Types The `types` list.
 */
template <types_concept Types> using types_unique_t = detail::types_unique_impl<Types>::type;

/**
 * @brief Alias for transforming a types list using a transform template.
 *
 * @tparam T The types list.
 * @tparam Transform The transform template.
 */
template <types_concept Types, types_transform Transform>
using types_transform_t = detail::types_transform_impl<Types, Transform>::type;

/**
 * @brief Alias for transforming a types list to a containers list.
 *
 * @tparam T The types list.
 * @tparam Container The container template.
 */
template <types_concept Types, types_container Container = types_containers::vector>
using types_to_containers_t = detail::types_to_containers<Types, Container>::type;

/**
 * @brief Gets the index of a type in a types list.
 *
 * @tparam Types The types list.
 * @tparam T The type to find.
 * @tparam Skip The number of occurrences to skip.
 */
template <types_concept Types, typename T, std::size_t Skip = 0>
inline constexpr std::size_t types_index_of_v = detail::types_index_of<T, Types, Skip>::value;

/**
 * @brief Alias for converting a types list to arrays of unique types.
 *
 * @tparam Types The types list.
 */
template <types_concept Types> using types_to_arrays_t = detail::types_to_arrays<Types, types_unique_t<Types>>::type;

/*
 * @brief Alias for removing type from a types list
 *
 * @tparam Types The types list.
 * @tparam T The type to remove
 */
template <types_concept Types, typename T> using types_remove_t = detail::types_remove<Types, T>::type;

template <types_concept Types, std::size_t N> using types_view_t = detail::types_view<N, Types>::type;

}

#endif
