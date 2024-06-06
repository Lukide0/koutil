#ifndef KOUTIL_CONTAINER_MULTI_VECTOR_H
#define KOUTIL_CONTAINER_MULTI_VECTOR_H

#include "koutil/type/types.h"
#include <cassert>
#include <cstddef>
#include <iterator>
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
concept is_multi_vector_element = !std::is_reference_v<T> && !std::is_void_v<T>;

/**
 * @brief Class representing a multi_vector.
 *
 * @tparam Types The types of elements stored in the multi_vector.
 */
template <is_multi_vector_element... Types>
    requires(sizeof...(Types) > 0)
class multi_vector {
private:
    using types     = type::types<Types...>;
    using storage_t = type::types_transform_t<
        type::types_to_containers_t<types, type::types_containers::vector>,
        type::types_transforms::tuple>;

    static constexpr auto helper_seq = std::make_index_sequence<types::size>();

public:
    /**
     * @brief Nested iterator class for multi_vector.
     *
     * @tparam is_const Whether the iterator is const.
     */
    template <bool is_const> class iterator;

    using value_ref_t       = std::tuple<Types&...>;
    using const_value_ref_t = std::tuple<const Types&...>;

    using value_t = std::tuple<Types...>;

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

    multi_vector& operator=(const multi_vector&) = default;
    multi_vector& operator=(multi_vector&&)      = default;

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
    template <typename... Args> auto emplace_back(Args&&... args) {
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

        std::get<I>(m_storage) = std::vector<vector_element>(count, std::get<I>(value));

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

        std::get<I>(m_storage) = std::vector<vector_element>(count);

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
        return std::tie(get_single<I>(pos)...);
    }

    /**
     * @brief Returns a tuple of const references to all elements at a specified position.
     *
     * @tparam I The indices of the types to return.
     * @param pos The position of the elements.
     * @return Tuple of const references to the elements.
     */
    template <std::size_t... I> const_value_ref_t get_all(std::size_t pos, std::index_sequence<I...> /*unused*/) const {
        return std::tie(get_single<I>(pos)...);
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
    auto emplace_back_impl(std::index_sequence<I...> /*unused*/, Args&&... args) {
        return std::tie(std::get<I>(m_storage).emplace_back(std::forward<decltype(args)>(args))...);
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
    requires(sizeof...(T) > 0)
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
