#ifndef KOUTIL_CONTAINER_HASH_ARRAY_H
#define KOUTIL_CONTAINER_HASH_ARRAY_H

#include "template_hash_array.h"
#include <cassert>
#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace koutil::container {

/**
 * @brief Concept to check if a type is a valid hash function for a given Key type.
 * @tparam T The hash function type.
 * @tparam Key The key type.
 */
template <typename T, typename Key>
concept is_hash = requires(T hash, const Key& key) {
    requires std::is_trivially_constructible_v<T>;

    { hash(key) } -> std::same_as<std::size_t>;
};

/**
 * @brief Concept to check if a type is a valid key adapter.
 * @tparam T The key adapter type.
 * @tparam Key The key type.
 * @tparam KeyID The key ID type.
 */
template <typename T, typename Key, typename KeyID>
concept is_key_adapter = requires(const Key& key, T adapter, const KeyID& index) {
    requires std::move_constructible<T> && std::is_trivially_copy_constructible_v<T>;
    { adapter.eql(key, index) } -> std::same_as<bool>;
};

/**
 * @brief A hash array with open addressing and linear probing.
 * @tparam Key The key type.
 * @tparam KeyID The key ID type.
 * @tparam KeyAdapter The key adapter type.
 * @tparam Hash The hash function type.
 * @tparam Bucket The bucket type.
 * @tparam Allocator The allocator type.
 */
template <
    typename Key,
    typename KeyID,
    is_key_adapter<Key, KeyID> KeyAdapter,
    is_hash<Key> Hash              = std::hash<Key>,
    is_bucket<KeyID> Bucket        = std::vector<std::pair<std::size_t, KeyID>>,
    is_allocator<Bucket> Allocator = std::allocator<Bucket>>
class hash_array {
private:
    using key_t       = Key;
    using key_id_t    = KeyID;
    using value_t     = std::size_t;
    using bucket_t    = Bucket;
    using hash_t      = Hash;
    using bucket_iter = bucket_t::iterator;
    using adapter_t   = KeyAdapter;
    using allocator_t = Allocator;

    struct AdapterWrapper {
        template <bool> bool eql(const key_t& key, const key_id_t& id) const { return adapter.eql(key, id); }

        adapter_t adapter;
    };

    struct HashWrapper {
        template <bool> std::size_t hash(const key_t& key) { return hasher(key); }

        hash_t hasher;
    };

    constexpr static bool comptime_value = true;

    using template_hash_array_t
        = template_hash_array<key_t, key_id_t, bool, AdapterWrapper, HashWrapper, bucket_t, allocator_t>;

public:
    using iterator_t       = template_hash_array_t::iterator_t;
    using const_iterator_t = template_hash_array_t::const_iterator_t;

    /**
     * @brief Default constructor.
     */
    hash_array() = default;

    /**
     * @brief Constructor with bucket count.
     *
     * @param bucket_count Number of buckets.
     */
    hash_array(std::size_t bucket_count)
        : m_storage(bucket_count) { }

    /**
     * @brief Copy constructor.
     *
     * @param other Another hash_array to copy from.
     */
    hash_array(const hash_array& other)
        : m_storage(other.m_storage) { }

    /**
     * @brief Move constructor.
     *
     * @param other Another hash_array to move from.
     */
    hash_array(hash_array&& other)
        : m_storage(std::move(other.m_storage))

    { }

    /**
     * @brief Destructor.
     */
    ~hash_array() = default;

    /**
     * @brief Copy assignment operator.
     *
     * @param other Another hash_array to copy from.
     * @return hash_array& Reference to the assigned hash_array.
     */
    hash_array& operator=(const hash_array& other) = default;
    /**
     * @brief Move assignment operator.
     *
     * @param other Another hash_array to move from.
     * @return hash_array& Reference to the assigned hash_array.
     */
    hash_array& operator=(hash_array&& other) = default;

    /**
     * @brief Check if the hash_array is empty.
     * @return bool True if empty, false otherwise.
     */
    [[nodiscard]] bool empty() const { return m_storage.empty(); }

    /**
     * @brief Get the number of elements in the hash_array.
     * @return std::size_t Number of elements.
     */
    [[nodiscard]] std::size_t size() const { return m_storage.size(); }

    /**
     * @brief Returns the number of buckets.
     * @return std::size_t Number of buckets.
     */
    [[nodiscard]] std::size_t bucket_count() const { return m_storage.bucket_count(); }

    /**
     * @brief Returns the maximum load factor.
     * @return float Maximum load factor.
     */
    [[nodiscard]] float max_load_factor() const { return m_storage.max_load_factor(); }

    /**
     * @brief Sets a new maximum load factor.
     * @param factor New maximum load factor.
     */
    void set_max_load_factor(float factor) {
        assert(factor > 0);
        m_storage.set_max_load_factor(factor);
    }

    /**
     * @brief Clear all elements from the hash_array.
     */
    void clear() { m_storage.clear(); }

    /**
     * @brief Try to insert a key and key ID into the hash_array.
     * @param key The key to insert.
     * @param key_id The key ID to insert.
     * @return bool True if the key is not inside hash_array, false otherwise.
     */
    bool try_insert(const key_t& key, const key_id_t& key_id, adapter_t adapter) {

        return m_storage.template try_insert<comptime_value>(key, key_id, AdapterWrapper { adapter });
    }

    /**
     * @brief Try to set new key ID.
     * @param key The key.
     * @param new_key_id The new key ID.
     * @return bool True if the key ID was changed, false otherwise.
     */
    bool try_set(const key_t& key, const key_id_t& new_key_id, adapter_t adapter) {
        return m_storage.template try_set<comptime_value>(key, new_key_id, AdapterWrapper { adapter });
    }

    /**
     * @brief Erases an element from the hash table.
     *
     * @param key Key of the element to erase.
     * @param adapter Key adapter for comparison.
     * @return true If the element was successfully erased.
     * @return false If the element was not found.
     */
    void erase(const key_t& key, adapter_t adapter) {
        m_storage.template erase<comptime_value>(key, AdapterWrapper { adapter });
    }

    /**
     * @brief Finds an element in the hash table.
     *
     * @param key Key of the element to find.
     * @param adapter Key adapter for comparison.
     * @return iterator_t The iterator with found element, if not found end() is returned.
     */
    iterator_t find(const key_t& key, adapter_t adapter) {
        return m_storage.template find<comptime_value>(key, AdapterWrapper { adapter });
    }

    /**
     * @brief Finds an element in the hash table.
     *
     * @param key Key of the element to find.
     * @param adapter Key adapter for comparison.
     * @return const_iterator_t The iterator with found element, if not found end() is returned.
     */
    const_iterator_t find(const key_t& key, adapter_t adapter) const {
        return m_storage.template find<comptime_value>(key, AdapterWrapper { adapter });
    }

    /**
     * @brief Get an iterator to the beginning of the hash_array.
     * @return iterator_t Iterator to the beginning.
     */
    iterator_t begin() { return m_storage.begin(); }

    /**
     * @brief Get an iterator to the end of the hash_array.
     * @return iterator_t Iterator to the end.
     */
    iterator_t end() { return m_storage.end(); }

    /**
     * @brief Get a constant iterator to the beginning of the hash_array.
     * @return const_iterator_t Constant iterator to the beginning.
     */
    const_iterator_t begin() const { return m_storage.begin(); }

    /**
     * @brief Get a constant iterator to the end of the hash_array.
     * @return const_iterator_t Constant iterator to the end.
     */
    const_iterator_t end() const { return m_storage.end(); }

    /**
     * @brief Get a constant iterator to the beginning of the hash_array.
     * @return iterator_t Constant iterator to the beginning.
     */
    const_iterator_t cbegin() const { return m_storage.cbegin(); }

    /**
     * @brief Get a constant iterator to the end of the hash_array.
     * @return const_iterator_t  Constant iterator to the end.
     */
    const_iterator_t cend() const { return m_storage.cend(); }

private:
    template_hash_array_t m_storage;
};
}

#endif
