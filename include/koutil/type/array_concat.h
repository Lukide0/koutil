#ifndef KOUTIL_TYPE_ARRAY_CONCAT_H
#define KOUTIL_TYPE_ARRAY_CONCAT_H

#include <array>
#include <cstddef>
#include <tuple>
#include <utility>

namespace koutil::type {

/**
 * @brief Concatenates multiple arrays into one array.
 *
 * @tparam T The type of the array elements.
 * @tparam Sizes The sizes of the arrays.
 * @param arrays The arrays to concatenate.
 * @return The concatenated array.
 */
template <typename T, std::size_t... Sizes> constexpr auto array_concat(const std::array<T, Sizes>&... arrays) {
    constexpr std::size_t size = (Sizes + ...);

    auto all = std::tuple_cat(arrays...);

    return [&]<std::size_t... I>(std::index_sequence<I...>) {
        return std::array<T, size> { { std::get<I>(all)... } };
    }(std::make_index_sequence<size>());
}

}

#endif
