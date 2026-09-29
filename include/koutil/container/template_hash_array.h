#ifndef KOUTIL_CONTAINER_TEMPLATE_HASH_ARRAY_H
#define KOUTIL_CONTAINER_TEMPLATE_HASH_ARRAY_H

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace koutil::container {

/**
 *
 * @brief Concept to check if a type is a valid hash function for a given Key type.
 * @tparam T The hash function type.
 * @tparam Key The key type.
 * @tparam ComptimeData The comptime data type.
 */
template <typename T, typename Key, typename ComptimeData>
concept is_template_hash = requires(T hash, const Key& key) {
    requires std::is_trivially_constructible_v<T>;

    { hash.template hash<ComptimeData { }>(key) } -> std::same_as<std::size_t>;
};

/**
 * @brief Concept to check if a type is a valid bucket type.
 * @tparam Bucket The bucket type.
 * @tparam KeyID The key ID type.
 */
template <typename Bucket, typename KeyID>
concept is_bucket = requires(Bucket& bucket, std::size_t v) {
    { *bucket.begin() } -> std::same_as<std::pair<std::size_t, KeyID>&>;
    { *bucket.end() } -> std::same_as<std::pair<std::size_t, KeyID>&>;
    { bucket.size() } -> std::same_as<std::size_t>;
    { bucket.emplace_back(v, v) };
    { bucket.clear() };
    { bucket.erase(bucket.begin()) };

    typename Bucket::iterator;

    requires std::forward_iterator<typename Bucket::iterator>;
    requires std::is_same_v<typename Bucket::iterator, decltype(bucket.begin())>
        && std::is_same_v<typename Bucket::iterator, decltype(bucket.end())>;
};

/**
 * @brief Concept to check if a type is a valid key adapter.
 * @tparam T The key adapter type.
 * @tparam Key The key type.
 * @tparam KeyID The key ID type.
 * @tparam ComptimeData The comptime data type.
 */
template <typename T, typename Key, typename KeyID, typename ComptimeData>
concept is_template_key_adapter = requires(const Key& key, T adapter, const KeyID& index) {
    requires std::move_constructible<T> && std::is_trivially_copy_constructible_v<T>;
    { adapter.template eql<ComptimeData { }>(key, index) } -> std::same_as<bool>;
};

template <
    typename Key,
    typename KeyID,
    typename ComptimeData,
    is_template_key_adapter<Key, KeyID, ComptimeData> KeyAdapter,
    is_template_hash<Key, ComptimeData> Hash,
    is_bucket<KeyID> Bucket = std::vector<std::pair<std::size_t, KeyID>>>
class template_hash_array {
private:
    using pair_t            = std::pair<std::size_t, KeyID>;
    using key_t             = Key;
    using key_id_t          = KeyID;
    using value_t           = std::size_t;
    using bucket_t          = Bucket;
    using hash_t            = Hash;
    using bucket_iter       = bucket_t::iterator;
    using bucket_const_iter = bucket_t::const_iterator;
    using adapter_t         = KeyAdapter;
    using allocator_t       = std::allocator<Bucket>;
    using comptime_t        = ComptimeData;

    /**
     * @brief Iterator class template for hash_array.
     *
     * @tparam is_const Boolean indicating if the iterator is constant.
     */
    template <bool is_const> class iterator {
    private:
        using ref_t = std::conditional_t<is_const, const bucket_t*, bucket_t*>;

    public:
        using value_type         = KeyID;
        using reference          = std::conditional_t<is_const, const value_type&, value_type&>;
        using constant_reference = const value_type&;

        using iterator_tag    = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;

        /**
         * @brief Default constructor.
         */
        iterator()
            : m_ref(nullptr) { }

        /**
         * @brief Constructor with parameters.
         *
         * @param bucket Pointer to the current bucket.
         * @param bucket_item Index of the current item in the bucket.
         * @param bucket_end Pointer to the end of the buckets.
         */
        iterator(ref_t bucket, std::size_t bucket_item, ref_t bucket_end)
            : m_ref(bucket)
            , m_item(bucket_item)
            , m_bucket_end(bucket_end) {
            find_non_empty();
        }

        iterator(iterator&&)                 = default;
        iterator(const iterator&)            = default;
        iterator& operator=(iterator&&)      = default;
        iterator& operator=(const iterator&) = default;

        /**
         * @brief Conversion operator to constant iterator.
         *
         * @return iterator<true> Constant iterator.
         */
        operator iterator<true>() const
            requires(!is_const)
        {
            return { m_ref, m_item, m_bucket_end };
        }

        bool operator==(const iterator& other) const { return m_ref == other.m_ref && m_item == other.m_item; }

        reference operator*() { return m_ref->at(m_item).second; }

        constant_reference operator*() const { return m_ref->at(m_item).second; }

        iterator& operator++() {
            increment();
            return *this;
        }

        iterator operator++(int) {
            iterator tmp = *this;

            increment();
            return tmp;
        }

    private:
        ref_t m_ref;
        std::size_t m_item = 0;
        ref_t m_bucket_end = nullptr;

        /**
         * @brief Helper function to increment the iterator.
         */
        void increment() {
            m_item += 1;
            if (m_item >= m_ref->size()) {
                m_item = 0;
                m_ref += 1;

                find_non_empty();
            }
        }

        void find_non_empty() {
            while (m_ref < m_bucket_end && m_ref->size() == 0) {
                m_ref += 1;
            }
        }
    };

public:
    using iterator_t       = iterator<false>;
    using const_iterator_t = iterator<true>;

    /**
     * @brief Default constructor.
     */
    template_hash_array() = default;

    /**
     * @brief Constructor with bucket count.
     *
     * @param bucket_count Number of buckets.
     */
    template_hash_array(std::size_t bucket_count)
        : m_buckets(create_buckets(bucket_count))
        , m_buckets_count(bucket_count) { }

    /**
     * @brief Copy constructor.
     *
     * @param other Another hash_array to copy from.
     */
    template_hash_array(const template_hash_array& other)
        : m_buckets(copy_buckets(other.m_buckets, other.m_buckets_count))
        , m_buckets_count(other.m_buckets_count)
        , m_size(other.m_size)
        , m_max_load_factor(other.m_max_load_factor) { }

    /**
     * @brief Move constructor.
     *
     * @param other Another hash_array to move from.
     */
    template_hash_array(template_hash_array&& other) noexcept
        : m_buckets(std::exchange(other.m_buckets, nullptr))
        , m_buckets_count(std::exchange(other.m_buckets_count, 0))
        , m_size(std::exchange(other.m_size, 0))
        , m_max_load_factor(other.m_max_load_factor) { }

    /**
     * @brief Destructor.
     */
    ~template_hash_array() { destroy(); }

    /**
     * @brief Copy assignment operator.
     *
     * @param other Another hash_array to copy from.
     * @return hash_array& Reference to the assigned hash_array.
     */
    template_hash_array& operator=(const template_hash_array& other) {
        if (&other == this) {
            return *this;
        }

        destroy();

        m_buckets_count   = other.m_buckets_count;
        m_size            = other.m_size;
        m_max_load_factor = other.m_max_load_factor;

        m_buckets = copy_buckets(other.m_buckets, other.m_buckets_count);
        return *this;
    }

    /**
     * @brief Move assignment operator.
     *
     * @param other Another hash_array to move from.
     * @return hash_array& Reference to the assigned hash_array.
     */
    template_hash_array& operator=(template_hash_array&& other) noexcept {
        if (&other == this) {
            return *this;
        }

        destroy();

        m_buckets         = std::exchange(other.m_buckets, nullptr);
        m_buckets_count   = std::exchange(other.m_buckets_count, 0);
        m_size            = std::exchange(other.m_size, 0);
        m_max_load_factor = other.m_max_load_factor;

        return *this;
    }

    /**
     * @brief Check if the hash_array is empty.
     * @return bool True if empty, false otherwise.
     */
    [[nodiscard]] bool empty() const { return m_size == 0; }

    /**
     * @brief Get the number of elements in the hash_array.
     * @return std::size_t Number of elements.
     */
    [[nodiscard]] std::size_t size() const { return m_size; }

    /**
     * @brief Returns the number of buckets.
     * @return std::size_t Number of buckets.
     */
    [[nodiscard]] std::size_t bucket_count() const { return m_buckets_count; }

    /**
     * @brief Returns the maximum load factor.
     * @return float Maximum load factor.
     */
    [[nodiscard]] float max_load_factor() const { return m_max_load_factor; }

    /**
     * @brief Sets a new maximum load factor.
     * @param factor New maximum load factor.
     */
    void set_max_load_factor(float factor) {
        assert(factor > 0);
        m_max_load_factor = factor;
    }

    /**
     * @brief Clear all elements from the hash_array.
     */
    void clear() { clear_all_buckets(); }

    /**
     * @brief Try to insert a key and key ID into the hash_array.
     * @tparam Data The comptime data.
     * @param key The key to insert.
     * @param key_id The key ID to insert.
     * @return bool True if the key is not inside hash_array, false otherwise.
     */
    template <comptime_t Data> bool try_insert(const key_t& key, const key_id_t& key_id, adapter_t adapter) {
        return insert<Data>(key, key_id, adapter);
    }

    /**
     * @brief Try to insert a key and key ID into the hash_array with already computed hash.
     * @tparam Data The comptime data.
     * @param key The key to insert.
     * @param key_id The key ID to insert.
     * @param key_hash The key hash.
     * @return bool True if the key is not inside hash_array, false otherwise.
     */
    template <comptime_t Data>
    bool try_insert(const key_t& key, const key_id_t& key_id, std::size_t key_hash, adapter_t adapter) {
        assert(hash_of<Data>(key) == key_hash);
        return insert<Data>(key, key_id, key_hash, adapter);
    }

    /**
     * @brief Try to set new key ID.
     * @tparam Data The comptime data.
     * @param key The key.
     * @param new_key_id The new key ID.
     * @return bool True if the key ID was changed, false otherwise.
     */
    template <comptime_t Data> bool try_set(const key_t& key, const key_id_t& new_key_id, adapter_t adapter) {
        return set<Data>(key, new_key_id, adapter);
    }

    /**
     * @brief Erases an element from the hash table.
     * @tparam Data The comptime data.
     * @param key Key of the element to erase.
     * @param adapter Key adapter for comparison.
     * @return true If the element was successfully erased.
     * @return false If the element was not found.
     */
    template <comptime_t Data> bool erase(const key_t& key, adapter_t adapter) { return remove<Data>(key, adapter); }

    /**
     * @brief Finds an element in the hash table.
     * @tparam Data The comptime data.
     * @param key Key of the element to find.
     * @param adapter Key adapter for comparison.
     * @return iterator_t The iterator with found element, if not found end() is returned.
     */
    template <comptime_t Data> iterator_t find(const key_t& key, adapter_t adapter) {
        return find_impl<Data>(key, hash_of<Data>(key), adapter);
    }

    /**
     * @brief Finds an element in the hash table.
     * @tparam Data The comptime data.
     * @param key Key of the element to find.
     * @param key_hash The hash of key.
     * @param adapter Key adapter for comparison.
     * @return iterator_t The iterator with found element, if not found end() is returned.
     */
    template <comptime_t Data> iterator_t find(const key_t& key, std::size_t key_hash, adapter_t adapter) {
        assert(hash_of<Data>(key) == key_hash);
        return find_impl<Data>(key, key_hash, adapter);
    }

    /**
     * @brief Finds an element in the hash table.
     * @tparam Data The comptime data.
     * @param key Key of the element to find.
     * @param adapter Key adapter for comparison.
     * @return const_iterator_t The iterator with found element, if not found end() is returned.
     */
    template <comptime_t Data> const_iterator_t find(const key_t& key, adapter_t adapter) const {
        return find_impl<Data>(key, hash_of<Data>(key), adapter);
    }

    /**
     * @brief Finds an element in the hash table.
     * @tparam Data The comptime data.
     * @param key Key of the element to find.
     * @param key_hash The hash of key.
     * @param adapter Key adapter for comparison.
     * @return const_iterator_t The iterator with found element, if not found end() is returned.
     */
    template <comptime_t Data> const_iterator_t find(const key_t& key, std::size_t key_hash, adapter_t adapter) const {
        assert(hash_of<Data>(key) == key_hash);
        return find_impl<Data>(key, key_hash, adapter);
    }

    /**
     * @brief Get an iterator to the beginning of the hash_array.
     * @return iterator_t Iterator to the beginning.
     */
    iterator_t begin() { return iterator_t { m_buckets, 0, m_buckets + m_buckets_count }; }

    /**
     * @brief Get an iterator to the end of the hash_array.
     * @return iterator_t Iterator to the end.
     */
    iterator_t end() { return iterator_t { m_buckets + m_buckets_count, 0, m_buckets + m_buckets_count }; }

    /**
     * @brief Get a constant iterator to the beginning of the hash_array.
     * @return const_iterator_t Constant iterator to the beginning.
     */
    const_iterator_t begin() const { return const_iterator_t { m_buckets, 0, m_buckets + m_buckets_count }; }

    /**
     * @brief Get a constant iterator to the end of the hash_array.
     * @return const_iterator_t Constant iterator to the end.
     */
    const_iterator_t end() const {
        return const_iterator_t { m_buckets + m_buckets_count, 0, m_buckets + m_buckets_count };
    }

    /**
     * @brief Get a constant iterator to the beginning of the hash_array.
     * @return iterator_t Constant iterator to the beginning.
     */
    const_iterator_t cbegin() const { return const_iterator_t { m_buckets, 0, m_buckets + m_buckets_count }; }

    /**
     * @brief Get a constant iterator to the end of the hash_array.
     * @return const_iterator_t  Constant iterator to the end.
     */
    const_iterator_t cend() const {
        return const_iterator_t { m_buckets + m_buckets_count, 0, m_buckets + m_buckets_count };
    }

private:
    [[no_unique_address]] allocator_t m_alloc;
    bucket_t* m_buckets         = nullptr;
    std::size_t m_buckets_count = 0;
    std::size_t m_size          = 0;
    float m_max_load_factor     = 1.0F;

    [[nodiscard]] value_t index_of_hash(value_t hash) const { return hash % m_buckets_count; }

    template <comptime_t Data> value_t hash_of(const Key& key) const { return hash_t().template hash<Data>(key); }

    template <comptime_t Data, typename B>
    static auto find_bucket_item(const key_t& key, std::size_t hash, B& bucket, adapter_t adapter) {
        return std::find_if(bucket.begin(), bucket.end(), [&](const auto& entry) {
            return entry.first == hash && adapter.template eql<Data>(key, entry.second);
        });
    }

    template <comptime_t Data> iterator_t find_impl(const key_t& key, std::size_t key_hash, adapter_t adapter) {
        if (m_buckets_count == 0) {
            return end();
        }

        auto& bucket = m_buckets[index_of_hash(key_hash)];
        auto it      = find_bucket_item<Data>(key, key_hash, bucket, adapter);

        if (it != bucket.end()) {
            return iterator_t { &bucket,
                                static_cast<std::size_t>(std::distance(bucket.begin(), it)),
                                m_buckets + m_buckets_count };
        }

        return end();
    }

    template <comptime_t Data>
    const_iterator_t find_impl(const key_t& key, std::size_t key_hash, adapter_t adapter) const {
        if (m_buckets_count == 0) {
            return end();
        }

        const auto& bucket = m_buckets[index_of_hash(key_hash)];
        auto it            = find_bucket_item<Data>(key, key_hash, bucket, adapter);

        if (it != bucket.end()) {
            return const_iterator_t { &bucket,
                                      static_cast<std::size_t>(std::distance(bucket.begin(), it)),
                                      m_buckets + m_buckets_count };
        }

        return end();
    }

    template <comptime_t Data> bool insert(const key_t& key, const key_id_t& key_id, adapter_t adapter) {
        return insert<Data>(key, key_id, hash_of<Data>(key), adapter);
    }

    template <comptime_t Data>
    bool insert(const key_t& key, const key_id_t& key_id, std::size_t key_hash, adapter_t adapter) {
        if (m_buckets_count == 0) {
            rehash(1);
        }

        auto& bucket = m_buckets[index_of_hash(key_hash)];

        auto it = find_bucket_item<Data>(key, key_hash, bucket, adapter);

        if (it != bucket.end()) {
            return false;
        }

        bucket.emplace_back(key_hash, key_id);
        m_size += 1;
        rehash_if_needed();
        return true;
    }

    template <comptime_t Data> bool remove(const key_t& key, adapter_t adapter) {
        if (m_buckets_count == 0) {
            return false;
        }

        auto hash    = hash_of<Data>(key);
        auto& bucket = m_buckets[index_of_hash(hash)];
        auto it      = find_bucket_item<Data>(key, hash, bucket, adapter);

        if (it == bucket.end()) {
            return false;
        }

        // special case
        if constexpr (std::is_same_v<bucket_t, std::vector<pair_t>>) {
            auto& it_val = *it;
            if (&it_val != &bucket.back()) {
                // replace removed element with last
                it_val = std::move(bucket.back());
            }

            // remove last element
            bucket.pop_back();
        } else {
            bucket.erase(it);
        }
        m_size -= 1;
        return true;
    }

    template <comptime_t Data> bool set(const key_t& key, const key_id_t& new_key_id, adapter_t adapter) {
        if (m_buckets_count == 0) {
            return false;
        }

        auto hash    = hash_of<Data>(key);
        auto& bucket = m_buckets[index_of_hash(hash)];
        auto it      = find_bucket_item<Data>(key, hash, bucket, adapter);

        if (it != bucket.end()) {
            it->second = new_key_id;
            return true;
        }
        return false;
    }

    bucket_t* create_buckets(std::size_t count) {
        if (count == 0) {
            return nullptr;
        }

        bucket_t* buckets = m_alloc.allocate(count);
        std::uninitialized_value_construct_n(buckets, count);

        return buckets;
    }

    bucket_t* copy_buckets(const bucket_t* src, std::size_t count) {
        if (count == 0) {
            return nullptr;
        }

        bucket_t* buckets = m_alloc.allocate(count);
        std::uninitialized_copy_n(src, count, buckets);

        return buckets;
    }

    void rehash_if_needed() {
        if (static_cast<float>(m_size) <= static_cast<float>(m_buckets_count) * m_max_load_factor) {
            return;
        }

        std::size_t new_buckets_count = m_buckets_count * 2;
        rehash(new_buckets_count);
    }

    void rehash(std::size_t count) {
        auto* buckets = create_buckets(count);

        for (std::size_t i = 0; i < m_buckets_count; ++i) {
            for (auto&& [hash, key_index] : m_buckets[i]) {
                buckets[hash % count].emplace_back(hash, key_index);
            }
        }

        if (m_buckets != nullptr) {
            std::destroy_n(m_buckets, m_buckets_count);
            m_alloc.deallocate(m_buckets, m_buckets_count);
        }

        m_buckets_count = count;
        m_buckets       = buckets;
    }

    void clear_all_buckets() {
        for (std::size_t i = 0; i < m_buckets_count; ++i) {
            m_buckets[i].clear();
        }
        m_size = 0;
    }

    void destroy() {

        if (m_buckets_count > 0) {
            std::destroy_n(m_buckets, m_buckets_count);
            m_alloc.deallocate(m_buckets, m_buckets_count);
        }

        m_buckets       = nullptr;
        m_buckets_count = 0;
        m_size          = 0;
    }
};

}

#endif
