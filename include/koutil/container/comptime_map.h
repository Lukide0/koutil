#ifndef KOUTIL_CONTAINER_COMPTIME_MAP_H
#define KOUTIL_CONTAINER_COMPTIME_MAP_H

#include "koutil/type/array_concat.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <limits>
#include <utility>

namespace koutil::container {

/**
 * @brief A compile-time map implementation.
 *
 * @tparam Key The key type.
 * @tparam Value The value type.
 * @tparam Size The size of the map.
 */
template <typename Key, typename Value, std::size_t Size> class ComptimeMap {
public:
    using pair_t               = std::pair<Key, Value>;
    using pairs_t              = std::array<pair_t, Size>;
    static constexpr auto npos = std::numeric_limits<std::size_t>::max();

    /**
     * @brief Constructs a compile-time map from an array of pairs.
     *
     * @param pairs The array of key-value pairs.
     */
    consteval ComptimeMap(pairs_t pairs)
        : m_data(std::move(pairs)) {

        if (contains_duplicate_key()) {
            assert(false && "Duplicate keys are not allowed in a compile-time map.");
        }
        std::ranges::sort(m_data, {}, &pair_t::first);
    }

    template <std::size_t Count> [[nodiscard]] consteval auto extend(const std::array<pair_t, Count>& pairs) const {
        return ComptimeMap<Key, Value, Size + Count>(type::array_concat(m_data, pairs));
    }

    [[nodiscard]] consteval auto extend(const pair_t& pair) const {
        return ComptimeMap<Key, Value, Size + 1>(type::array_concat(m_data, std::array<pair_t, 1>({ pair })));
    }

    template <std::size_t OtherSize>
    [[nodiscard]] consteval auto extend(const ComptimeMap<Key, Value, OtherSize>& other) const {
        return extend(other.m_data);
    }

    /**
     * @brief Checks if there are duplicate keys in the map.
     *
     * @return True if duplicate keys are found, false otherwise.
     */
    [[nodiscard]] consteval bool contains_duplicate_key() const {
        for (std::size_t i = 0; i < Size; i++) {
            for (std::size_t j = i + 1; j < Size; j++) {
                if (m_data[i].first == m_data[j].first) {
                    return true;
                }
            }
        }

        return false;
    }

    /**
     * @brief Finds the index of a key in the map.
     *
     * @param key The key to find.
     * @return The index of the key if found, npos otherwise.
     */
    [[nodiscard]] constexpr std::size_t find(const Key& key) const {
        const auto iter = std::ranges::lower_bound(m_data.begin(), m_data.end(), key, {}, &pair_t::first);
        if (iter == m_data.end()) {
            return npos;
        } else {
            return std::distance(m_data.begin(), iter);
        }
    }

    /**
     * @brief Retrieves the value associated with a key.
     *
     * @param key The key.
     * @return The value associated with the key.
     */
    [[nodiscard]] constexpr const Value& operator[](const Key& key) const { return get(key); }

    /**
     * @brief Retrieves the value associated with a key.
     *
     * @param key The key.
     * @return The value associated with the key.
     */
    [[nodiscard]] constexpr const Value& at(const Key& key) const {
        const auto index = find(key);
        return unsafe_get(index);
    }

    /**
     * @brief Retrieves the value associated with a key or a default value if not found.
     *
     * @param key The key.
     * @param default_value The default value.
     * @return The value associated with the key if found, default_value otherwise.
     */
    [[nodiscard]] constexpr const Value& safe_at(const Key& key, const Value& default_value) const {
        const auto index = find(key);
        return (index == npos) ? default_value : unsafe_get(index);
    }

    /**
     * @brief Retrieves the value at a specific index.
     *
     * @param index The index.
     * @return The value at the specified index.
     */
    [[nodiscard]] constexpr const Value& unsafe_get(std::size_t index) const { return m_data[index].second; }

    /**
     * @brief Retrieves the key-value pair associated with a key.
     *
     * @param key The key.
     * @return The key-value pair.
     */
    [[nodiscard]] constexpr const pair_t& get_pair(const Key& key) const { return m_data[find(key)]; }

    /**
     * @brief Checks if the map contains a key.
     *
     * @param key The key to check.
     * @return True if the map contains the key, false otherwise.
     */
    [[nodiscard]] constexpr bool contains(const Key& key) const { return find(key) != npos; }

    /**
     * @brief Checks if a specific value exists in the map.
     *
     * @param value The value to check.
     * @return True if the value exists, false otherwise.
     */
    [[nodiscard]] consteval bool test_value(const Value& value) const {
        for (auto&& [_, val] : m_data) {
            if (val == value) {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief Checks if a specific value does not exist in the map.
     *
     * @param value The value to check.
     * @return True if the value does not exist, false otherwise.
     */
    [[nodiscard]] consteval bool test_no_value(const Value& value) const { return !test_value(value); }

private:
    pairs_t m_data;
};

/**
 * @brief Converts an array of pairs to a compile-time map.
 * @tparam Key The key type.
 * @tparam Value The value type.
 * @tparam Size The size of the array.
 * @param pairs The array of key-value pairs.
 * @return The compile-time map.
 */
template <typename Key, typename Value, std::size_t Size>
consteval ComptimeMap<Key, Value, Size> to_map(std::array<std::pair<Key, Value>, Size> pairs) {
    return { pairs };
}

/**
 * @brief Converts an array of pairs to a compile-time map.
 * @tparam Key The key type.
 * @tparam Value The value type.
 * @tparam Size The size of the array.
 * @param pairs The array of key-value pairs.
 * @return The compile-time map.
 */
template <typename Key, typename Value, std::size_t Size>
// NOLINTNEXTLINE(modernize-avoid-c-arrays)
consteval ComptimeMap<Key, Value, Size> to_map(std::pair<Key, Value> (&&pairs)[Size]) {
    return { std::to_array(pairs) };
}

}

#endif
