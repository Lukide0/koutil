#ifndef KOUTIL_CONTAINER_MULTI_VECTOR_H
#define KOUTIL_CONTAINER_MULTI_VECTOR_H

#include "koutil/type/types.h"
#include <cassert>
#include <cstddef>
#include <iterator>
#include <memory>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace koutil::container {

/**
 * @brief Concept to check if a type is a valid multi_vector element.
 *
 * @tparam T The type to check.
 */
template <typename T>
concept is_multi_vector_element = !std::is_reference_v<T>;

/**
 * @brief Class representing a multi_vector.
 *
 * @tparam Types The types of elements stored in the multi_vector.
 */
template <is_multi_vector_element... Types>
    requires(type::types_remove_t<type::types<Types...>, void>::size != 0)
class multi_vector {
private:
    template <typename T, typename Allocator = std::allocator<T>> class single_vector {
    public:
        single_vector(std::size_t n)
            : m_data(Allocator().allocate(n))
            , m_capacity(n)
            , m_size(n) {
            std::uninitialized_fill_n(m_data, n, T());
        }

        single_vector(std::size_t n, const T& value)
            : m_data(Allocator().allocate(n))
            , m_capacity(n)
            , m_size(n) {
            std::uninitialized_fill_n(m_data, n, value);
        }

        single_vector() = default;

        single_vector(const single_vector& other)
            : m_data(Allocator().allocate(other.m_capacity))
            , m_capacity(other.m_capacity)
            , m_size(other.m_size) {
            std::uninitialized_copy_n(other.m_data, other.m_size, m_data);
        }

        ~single_vector() {
            if (m_capacity != 0) {
                std::destroy_n(m_data, m_size);
                Allocator().deallocate(m_data, m_capacity);
            }
        }

        single_vector(single_vector&& other)
            : m_data(other.m_data)
            , m_capacity(other.m_capacity)
            , m_size(other.m_size) {
            other.m_capacity = 0;
            other.m_size     = 0;
            other.m_data     = nullptr;
        }

        single_vector& operator=(single_vector&& other) {
            if (m_capacity != 0) {
                std::destroy_n(m_data, m_size);
                Allocator().deallocate(m_data, m_capacity);
            }

            m_size     = other.m_size;
            m_capacity = other.m_capacity;
            m_data     = other.m_data;

            other.m_size     = 0;
            other.m_capacity = 0;
            other.m_data     = nullptr;

            return *this;
        }

        single_vector& operator=(const single_vector& other) {
            if (&other == this) {
                return *this;
            }

            std::destroy_n(m_data, m_size);
            if (m_capacity != other.m_capacity) {
                auto alloc = Allocator();

                alloc.deallocate(m_data, m_capacity);

                m_data = new (alloc.allocate(other.m_capacity)) T[other.m_capacity];
            }

            std::uninitialized_copy_n(other.m_data, other.m_size, m_data);

            m_capacity = other.m_capacity;
            m_size     = other.m_size;

            return *this;
        }

        void clear() { m_size = 0; }

        void swap(single_vector& other) {
            std::swap(m_data, other.m_data);
            std::swap(m_size, other.m_size);
            std::swap(m_capacity, other.m_capacity);
        }

        void push_back(const T& value) {
            try_update_capacity();

            m_data[m_size] = value;
            m_size += 1;
        }

        void push_back(T&& value) {
            try_update_capacity();

            m_data[m_size] = std::move(value);
            m_size += 1;
        }

        template <typename Arg> T& emplace_back(Arg&& arg) {
            try_update_capacity();

            m_data[m_size] = T(std::forward<decltype(arg)>(arg));
            m_size += 1;

            return m_data[m_size - 1];
        }

        T* erase(const T* element) {
            assert(element <= m_data + m_size && element >= m_data);

            // last element
            if (m_data + m_size == element) {
                m_size -= 1;
                return m_data + m_size;
            }

            m_size -= 1;
            if (m_size == m_capacity / 2) {
                m_capacity /= 2;
            }

            const auto index = static_cast<std::size_t>(element - m_data);

            realloc_without(m_capacity, index);

            return m_data + index;
        }

        void pop_back() {
            assert(m_size > 0);
            m_size -= 1;
        }

        void resize(std::size_t n) { resize(n, {}); }

        void resize(std::size_t n, const T& value) {
            if (m_size >= n) {
                m_size = n;
                return;
            }

            if (m_capacity >= n) {
                std::uninitialized_fill_n(m_data + m_size, m_capacity - m_size, value);
            } else {
                realloc_with_value(n, value);
            }
            m_size = n;
        }

        void reserve(std::size_t n) {
            if (n <= m_capacity) {
                return;
            }

            realloc(n);
        }

        T& at(std::size_t i) { return m_data[i]; }

        [[nodiscard]] const T& at(std::size_t i) const { return m_data[i]; }

        T* data() { return m_data; }

        [[nodiscard]] const T* data() const { return m_data; }

        [[nodiscard]] std::size_t size() const { return m_size; }

        T* begin() { return m_data; }

        const T* begin() const { return m_data; }

        T* end() { return m_data + m_size; }

        const T* end() const { return m_data + m_size; }

    private:
        T* m_data              = nullptr;
        std::size_t m_capacity = 0;
        std::size_t m_size     = 0;

        [[nodiscard]] std::size_t next_capacity() const {
            if (m_capacity == 0) {
                return 1;
            } else [[likely]] {
                return m_capacity * 2;
            }
        }

        void try_update_capacity() {
            if (m_size + 1 <= m_capacity) {
                return;
            }
            realloc(next_capacity());
        }

        void realloc(std::size_t capacity) {
            const std::size_t old_capacity = m_capacity;
            m_capacity                     = capacity;

            auto alloc     = Allocator();
            auto* new_data = new (alloc.allocate(m_capacity)) T[m_capacity];

            std::uninitialized_move(m_data, m_data + m_size, new_data);

            std::destroy_n(m_data, m_size);
            alloc.deallocate(m_data, old_capacity);
            m_data = new_data;
        }

        void realloc_with_value(std::size_t capacity, const T& value = {}) {
            realloc(capacity);
            std::uninitialized_fill_n(m_data + m_size, m_capacity - m_size, value);
        }

        void realloc_without(std::size_t capacity, std::size_t index) {
            const std::size_t old_capacity = m_capacity;
            m_capacity                     = capacity;

            auto alloc  = Allocator();
            T* new_data = nullptr;

            if (m_capacity != 0) {
                new_data = new (alloc.allocate(m_capacity)) T[m_capacity];
                std::uninitialized_move_n(m_data, index, new_data);
                std::uninitialized_move_n(m_data + index + 1, m_size - index, new_data);
            }

            std::destroy_n(m_data, m_size + 1);
            alloc.deallocate(m_data, old_capacity);
            m_data = new_data;
        }
    };

    struct single_vector_container {
        template <typename T> using container = single_vector<T>;
    };

    using transforms = type::types_transforms;
    using containers = type::types_containers;

    template <type::types_concept T, type::types_container Container>
    using to_containers_t = type::types_to_containers_t<T, Container>;

    template <type::types_concept T, type::types_transform Transform>
    using transform_types_t = type::types_transform_t<T, Transform>;

    using all_types = type::types<Types...>;
    using types     = type::types_remove_t<all_types, void>;

    using storage_t = transform_types_t<to_containers_t<types, single_vector_container>, transforms::tuple>;

    static constexpr auto helper_seq = std::make_index_sequence<types::size>();

public:
    /**
     * @brief Nested iterator class for multi_vector.
     *
     * @tparam is_const Whether the iterator is const.
     */
    template <bool is_const> class iterator;

    using used_types = types;

    using value_ref_t = transform_types_t<transform_types_t<types, transforms::reference>, transforms::tuple>;
    using const_value_ref_t
        = transform_types_t<transform_types_t<types, transforms::constant_reference>, transforms::tuple>;

    using value_t = transform_types_t<types, transforms::tuple>;

    using iterator_t       = iterator<false>;
    using const_iterator_t = iterator<true>;

    multi_vector()                    = default;
    multi_vector(const multi_vector&) = default;
    multi_vector(multi_vector&&)      = default;

    /**
     * @brief Constructs a multi_vector with a specified size and initial value.
     *
     * @param count The number of elements.
     * @param value The initial value for the elements.
     */
    multi_vector(std::size_t count, const value_t& value) { init_vector(count, value); }

    /**
     * @brief Constructs a multi_vector with a specified size and default-initialized elements.
     *
     * @param count The number of elements.
     */
    explicit multi_vector(std::size_t count) { init_vector_default(count); }

    multi_vector& operator=(const multi_vector& other) = default;

    multi_vector& operator=(multi_vector&&) = default;

    template <std::size_t I> decltype(auto) get_container() {

        static_assert(!std::is_same_v<type::types_get_t<all_types, I>, void>, "This type was removed");

        using view                       = type::types_view_t<all_types, I>;
        constexpr std::size_t count_void = type::types_count<view>::template value<void>;
        constexpr std::size_t index      = view::size - count_void;

        using element = type::types_get_t<used_types, index>;

        auto& container = std::get<index>(m_storage);

        if constexpr (std::is_same_v<element, bool>) {
            return std::span<bool>(container.begin(), container.end());
        } else {
            return std::span { container };
        }
    }

    template <std::size_t I> decltype(auto) get_container() const {

        static_assert(!std::is_same_v<type::types_get_t<all_types, I>, void>, "This type was removed");

        using view                       = type::types_view_t<all_types, I>;
        constexpr std::size_t count_void = type::types_count<view>::template value<void>;
        constexpr std::size_t index      = view::size - count_void;

        using element = type::types_get_t<used_types, index>;

        const auto& container = std::get<index>(m_storage);

        if constexpr (std::is_same_v<element, bool>) {
            return std::span<bool>(container.begin(), container.end());
        } else {
            return std::span { container };
        }
    }

    /**
     * @brief Returns a reference to the first element.
     *
     * @return Reference to the first element.
     */
    value_ref_t front() {
        assert(!empty());
        return at(0);
    }

    /**
     * @brief Returns a const reference to the first element.
     *
     * @return Const reference to the first element.
     */
    const_value_ref_t front() const {
        assert(!empty());
        return at(0);
    }

    /**
     * @brief Returns a reference to the last element.
     *
     * @return Reference to the last element.
     */
    value_ref_t back() {
        assert(!empty());
        return at(size() - 1);
    }

    /**
     * @brief Returns a const reference to the last element.
     *
     * @return Const reference to the last element.
     */
    const_value_ref_t back() const {
        assert(!empty());
        return at(size() - 1);
    }

    /**
     * @brief Returns a reference to the element at a specified position.
     *
     * @param pos The position of the element.
     * @return Reference to the element.
     */
    value_ref_t at(std::size_t pos) {
        assert(pos < size());
        return get_all(pos, helper_seq);
    }

    /**
     * @brief Returns a const reference to the element at a specified position.
     *
     * @param pos The position of the element.
     * @return Const reference to the element.
     */
    const_value_ref_t at(std::size_t pos) const {
        assert(pos < size());
        return get_all(pos, helper_seq);
    }

    /**
     * @brief Returns a reference to the element at a specified position.
     *
     * @param pos The position of the element.
     * @return Reference to the element.
     */
    value_ref_t operator[](std::size_t pos) { return at(pos); }

    /**
     * @brief Returns a const reference to the element at a specified position.
     *
     * @param pos The position of the element.
     * @return Const reference to the element.
     */
    const_value_ref_t operator[](std::size_t pos) const { return at(pos); }

    /**
     * @brief Returns the number of elements in the multi_vector.
     *
     * @return The number of elements.
     */
    [[nodiscard]] std::size_t size() const { return std::get<0>(m_storage).size(); }

    /**
     * @brief Checks if the multi_vector is empty.
     *
     * @return True if the multi_vector is empty, false otherwise.
     */
    [[nodiscard]] bool empty() const { return size() == 0; }

    /**
     * @brief Clears the multi_vector.
     */
    void clear() { clear_all(helper_seq); }

    /**
     * @brief Swaps the contents of two multi_vectors.
     *
     * @param other The other multi_vector.
     */
    void swap(multi_vector& other) { std::swap(other.m_storage, m_storage); }

    /**
     * @brief Resizes the multi_vector to a specified size.
     *
     * @param size The new size.
     */
    void resize(std::size_t size) { resize_all(size, helper_seq); }

    /**
     * @brief Resizes the multi_vector to a specified size and initializes new elements with a specified value.
     *
     * @param size The new size.
     * @param value The value for new elements.
     */
    void resize(std::size_t size, const value_t& value) { resize_all_with(size, value, helper_seq); }

    /**
     * @brief Reserves storage for the multi_vector.
     *
     * @param size The amount of storage to reserve.
     */
    void reserve(std::size_t size) { reserve_impl(size, helper_seq); }

    /**
     * @brief Removes the last element from the multi_vector.
     */
    void pop_back() { pop_back_impl(helper_seq); }

    /**
     * @brief Adds a new element to the end of the multi_vector.
     *
     * @tparam Args The types of the arguments.
     * @param args The arguments to construct the new element.
     * @return A tuple of references to the newly added elements.
     */
    template <typename... Args>
        requires(sizeof...(Args) == types::size)
    value_ref_t emplace_back(Args&&... args) {
        return emplace_back_impl(helper_seq, std::forward<decltype(args)>(args)...);
    }

    /**
     * @brief Adds a new element to the end of the multi_vector.
     *
     * @param value The value to add.
     */
    void push_back(const value_t& value) { push_back_impl(value, helper_seq); }

    /**
     * @brief Adds a new element to the end of the multi_vector.
     *
     * @param value The value to add.
     */
    void push_back(value_t&& value) { push_back_impl(std::move(value), helper_seq); }

    /**
     * @brief Shrinks the capacity of the multi_vector to fit its size.
     */
    void shrink_to_fit() { shrink_to_fit_impl(helper_seq); }

    /**
     * @brief Erases an element at a specified position.
     *
     * @param pos The position of the element to erase.
     * @return An iterator to the element following the erased element.
     */
    iterator_t erase(const_iterator_t pos) {
        erase_impl(pos);

        const auto dist = static_cast<std::size_t>(pos - begin());
        if (dist >= size()) {
            return end();
        } else {
            return iterator_t { this, dist };
        }
    }

    /**
     * @brief Returns an iterator to the beginning of the multi_vector.
     *
     * @return An iterator to the beginning of the multi_vector.
     */
    iterator_t begin() { return iterator_t { this, 0 }; }

    /**
     * @brief Returns a const iterator to the beginning of the multi_vector.
     *
     * @return A const iterator to the beginning of the multi_vector.
     */
    const_iterator_t begin() const { return const_iterator_t { this, 0 }; }

    /**
     * @brief Returns a const iterator to the beginning of the multi_vector.
     *
     * @return A const iterator to the beginning of the multi_vector.
     */
    const_iterator_t cbegin() const { return const_iterator_t { this, 0 }; }

    /**
     * @brief Returns an iterator to the end of the multi_vector.
     *
     * @return An iterator to the end of the multi_vector.
     */
    iterator_t end() { return iterator_t { this, size() }; }

    /**
     * @brief Returns a const iterator to the end of the multi_vector.
     *
     * @return A const iterator to the end of the multi_vector.
     */
    const_iterator_t end() const { return const_iterator_t { this, size() }; }

    /**
     * @brief Returns a const iterator to the end of the multi_vector.
     *
     * @return A const iterator to the end of the multi_vector.
     */
    const_iterator_t cend() const { return const_iterator_t { this, size() }; }

private:
    storage_t m_storage;

    /**
     * @brief Initializes the multi_vector with a specified size and value.
     *
     * @tparam I The index of the type to initialize.
     * @param count The number of elements.
     * @param value The value to initialize the elements with.
     */
    template <std::size_t I = 0> void init_vector(std::size_t count, const value_t& value) {
        using vector_element = type::types_get_t<types, I>;

        std::get<I>(m_storage) = single_vector<vector_element>(count, std::get<I>(value));

        if constexpr (I + 1 < types::size) {
            init_vector<I + 1>(count, value);
        }
    }

    /**
     * @brief Initializes the multi_vector with a specified size and default values.
     *
     * @tparam I The index of the type to initialize.
     * @param count The number of elements.
     */
    template <std::size_t I = 0> void init_vector_default(std::size_t count) {
        using vector_element = type::types_get_t<types, I>;

        std::get<I>(m_storage) = single_vector<vector_element>(count);

        if constexpr (I + 1 < types::size) {
            init_vector_default<I + 1>(count);
        }
    }

    /**
     * @brief Returns a reference to a single element at a specified position.
     *
     * @tparam I The index of the type to return.
     * @param pos The position of the element.
     * @return Reference to the element.
     */
    template <std::size_t I> auto& get_single(std::size_t pos) { return std::get<I>(m_storage).at(pos); }

    /**
     * @brief Returns a const reference to a single element at a specified position.
     *
     * @tparam I The index of the type to return.
     * @param pos The position of the element.
     * @return Const reference to the element.
     */
    template <std::size_t I> const auto& get_single(std::size_t pos) const { return std::get<I>(m_storage).at(pos); }

    /**
     * @brief Returns a tuple of references to all elements at a specified position.
     *
     * @tparam I The indices of the types to return.
     * @param pos The position of the elements.
     * @return Tuple of references to the elements.
     */
    template <std::size_t... I> value_ref_t get_all(std::size_t pos, std::index_sequence<I...> /*unused*/) {
        return { get_single<I>(pos)... };
    }

    /**
     * @brief Returns a tuple of const references to all elements at a specified position.
     *
     * @tparam I The indices of the types to return.
     * @param pos The position of the elements.
     * @return Tuple of const references to the elements.
     */
    template <std::size_t... I> const_value_ref_t get_all(std::size_t pos, std::index_sequence<I...> /*unused*/) const {
        return { get_single<I>(pos)... };
    }

    /**
     * @brief Clears all containers in the multi_vector.
     *
     * @tparam I The indices of the types to clear.
     */
    template <std::size_t... I> void clear_all(std::index_sequence<I...> /*unused*/) {
        (std::get<I>(m_storage).clear(), ...);
    }

    /**
     * @brief Resizes all containers in the multi_vector.
     *
     * @tparam I The indices of the types to resize.
     * @param size The new size.
     */
    template <std::size_t... I> void resize_all(std::size_t size, std::index_sequence<I...> /*unused*/) {
        (std::get<I>(m_storage).resize(size), ...);
    }

    /**
     * @brief Resizes all containers in the multi_vector and initializes new elements with a specified value.
     *
     * @tparam I The indices of the types to resize.
     * @param size The new size.
     * @param value The value for new elements.
     */
    template <std::size_t... I>
    void resize_all_with(std::size_t size, const value_t& value, std::index_sequence<I...> /*unused*/) {
        (std::get<I>(m_storage).resize(size, std::get<I>(value)), ...);
    }

    /**
     * @brief Removes the last element from all containers in the multi_vector.
     *
     * @tparam I The indices of the types to pop back.
     */
    template <std::size_t... I> void pop_back_impl(std::index_sequence<I...> /*unused*/) {
        (std::get<I>(m_storage).pop_back(), ...);
    }

    /**
     * @brief Adds a new element to the end of all containers in the multi_vector.
     *
     * @tparam Value The type of the value to add.
     * @tparam I The indices of the types to push back.
     * @param value The value to add.
     */
    template <typename Value, std::size_t... I>
    void push_back_impl(Value&& value, std::index_sequence<I...> /*unused*/) {
        (std::get<I>(m_storage).push_back(std::get<I>(std::forward<decltype(value)>(value))), ...);
    }

    /**
     * @brief Adds a new element to the end of all containers in the multi_vector.
     *
     * @tparam Args The types of the arguments.
     * @tparam I The indices of the types to emplace back.
     * @param args The arguments to construct the new elements.
     * @return A tuple of references to the newly added elements.
     */
    template <typename... Args, std::size_t... I>
    value_ref_t emplace_back_impl(std::index_sequence<I...> /*unused*/, Args&&... args) {
        return std::forward_as_tuple(std::get<I>(m_storage).emplace_back(std::forward<decltype(args)>(args))...);
    }

    /**
     * @brief Shrinks the capacity of all containers in the multi_vector to fit their size.
     *
     * @tparam I The indices of the types to shrink to fit.
     */
    template <std::size_t... I> void shrink_to_fit_impl(std::index_sequence<I...> /*unused*/) {
        (std::get<I>(m_storage).shrink_to_fit(), ...);
    }

    /**
     * @brief Reserves storage for all containers in the multi_vector.
     *
     * @tparam I The indices of the types to reserve.
     * @param size The amount of storage to reserve.
     */
    template <std::size_t... I> void reserve_impl(std::size_t size, std::index_sequence<I...> /*unused*/) {
        (std::get<I>(m_storage).reserve(size), ...);
    }

    /**
     * @brief Erases an element at a specified position from all containers in the multi_vector.
     *
     * @tparam I The index of the type to erase.
     * @param pos The position of the element to erase.
     */
    template <std::size_t I = 0> void erase_impl(const_iterator_t pos) {
        auto& vec = std::get<I>(m_storage);
        vec.erase(vec.begin() + pos.m_index);

        if constexpr (I + 1 < types::size) {
            erase_impl<I + 1>(pos);
        }
    }
};

/**
 * @brief Nested iterator class for multi_vector.
 *
 * @tparam T The types of elements stored in the multi_vector.
 * @tparam is_const Whether the iterator is const.
 */
template <is_multi_vector_element... T>
    requires(type::types_remove_t<type::types<T...>, void>::size != 0)
template <bool is_const>
class multi_vector<T...>::iterator {
private:
    using vec_ptr_t = std::conditional_t<is_const, const multi_vector*, multi_vector*>;

public:
    using value_type        = std::conditional_t<is_const, const_value_ref_t, value_ref_t>;
    using const_value_type  = const_value_ref_t;
    using reference         = value_type;
    using difference_type   = std::ptrdiff_t;
    using iterator_category = std::random_access_iterator_tag;

    iterator()                           = default;
    iterator(const iterator&)            = default;
    iterator(iterator&&)                 = default;
    iterator& operator=(const iterator&) = default;
    iterator& operator=(iterator&&)      = default;

    const_value_type operator*() const { return m_ref->at(m_index); }

    value_type operator*()
        requires(!is_const)
    {
        return m_ref->at(m_index);
    }

    iterator& operator++() {
        m_index++;
        return *this;
    }

    operator iterator<true>() const
        requires(!is_const)
    {
        return iterator<true>(m_ref, m_index);
    }

    iterator operator++(int) { return { m_ref, m_index++ }; }

    iterator& operator--() {
        --m_index;
        return *this;
    }

    iterator& operator--(int) { return { m_ref, m_index-- }; }

    difference_type operator-(const iterator& other) const {
        if (m_index < other.m_index) {
            return -static_cast<difference_type>(other.m_index - m_index);
        } else {
            return static_cast<difference_type>(m_index - other.m_index);
        }
    }

    iterator& operator+=(difference_type n) {
        m_index += n;
        return *this;
    }

    iterator& operator-=(difference_type n) {
        m_index -= n;
        return *this;
    }

    iterator operator+(difference_type n) const { return { m_ref, m_index + n }; }

    iterator operator-(difference_type n) const { return { m_ref, m_index - n }; }

    const_value_type operator[](difference_type n) const { return m_ref->at(m_index + n); }

    value_type operator[](difference_type n)
        requires(!is_const)
    {
        return m_ref->at(m_index + n);
    }

    bool operator==(const iterator& other) const = default;
    bool operator!=(const iterator& other) const = default;

    bool operator<(const iterator& other) const { return m_index < other.m_index; }

    bool operator<=(const iterator& other) const { return m_index <= other.m_index; }

    bool operator>(const iterator& other) const { return m_index > other.m_index; }

    bool operator>=(const iterator& other) const { return m_index >= other.m_index; }

private:
    vec_ptr_t m_ref     = nullptr;
    std::size_t m_index = 0;

    iterator(vec_ptr_t vec, std::size_t index)
        : m_ref(vec)
        , m_index(index) { }

    friend class iterator<true>;
    friend class multi_vector;
};
}

#endif
