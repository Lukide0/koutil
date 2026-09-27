#ifndef KOUTIL_CONTAINER_SMALL_VECTOR_H
#define KOUTIL_CONTAINER_SMALL_VECTOR_H

#include <algorithm>
#include <cstddef>
#include <memory>
#include <new>

namespace koutil::container {

template <typename T, std::size_t Size> class stack_buffer_t {
public:
    T* data() { return std::launder(reinterpret_cast<T*>(m_stack)); }

    const T* data() const { return std::launder(reinterpret_cast<const T*>(m_stack)); }

    [[nodiscard]] std::size_t size() const { return Size; }

private:
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    alignas(T) std::byte m_stack[sizeof(T) * Size];
};

template <typename T> class stack_buffer_t<T, 0> {
public:
    T* data() { return nullptr; }

    const T* data() const { return nullptr; }

    [[nodiscard]] std::size_t size() const { return 0; }
};

/**
 * @brief Small vector with configurable stack storage.
 *
 * @tparam T Element type.
 * @tparam OnStack Number of elements stored in the stack buffer.
 */
template <typename T, std::size_t OnStack = 0> class small_vector {
public:
    using value_t          = T;
    using allocator_t      = std::allocator<T>;
    using stack_t          = stack_buffer_t<T, OnStack>;
    using alloc_traits     = std::allocator_traits<allocator_t>;
    using iterator_t       = value_t*;
    using const_iterator_t = const value_t*;

    /** @brief Constructs an empty vector. */
    small_vector()
        : m_stack()
        , m_alloc()
        , m_size(0)
        , m_capacity(m_stack.size())
        , m_data(m_stack.data()) { }

    /** @brief Constructs a copy of another vector. */
    small_vector(const small_vector& other) {
        reserve_empty(other.m_size);

        std::uninitialized_copy_n(other.m_data, other.m_size, m_data);
        m_size = other.m_size;
    }

    /** @brief Constructs a vector by moving another vector. */
    small_vector(small_vector&& other)
        : m_alloc(std::move(other.m_alloc))
        , m_size(0)
        , m_capacity(m_stack.size())
        , m_data(m_stack.data()) {

        if (other.uses_stack()) {
            reserve_empty(other.m_size);

            std::uninitialized_move_n(other.m_data, other.m_size, m_data);
            m_size = other.m_size;

            other.clear();
        } else {
            m_capacity = other.m_capacity;
            m_size     = other.m_size;
            m_data     = other.m_data;
            m_alloc    = std::move(other.m_alloc);

            other.m_data     = other.m_stack.data();
            other.m_size     = 0;
            other.m_capacity = other.m_stack.size();
        }
    }

    /**
     * @brief Constructs a vector with a given number of copies of a value.
     *
     * @param count Number of elements.
     * @param value Value used to initialize the elements.
     */
    explicit small_vector(std::size_t count, const value_t& value = value_t { })
        : small_vector() {
        reserve_empty(count);

        std::uninitialized_fill_n(m_data, count, value);
        m_size = count;
    }

    small_vector& operator=(const small_vector& other) {
        if (this == &other) {
            return *this;
        }

        // we don't need another heap allocation
        if (m_capacity >= other.m_size) {

            const std::size_t common = std::min(m_size, other.m_size);

            // copy constructor
            std::copy_n(other.m_data, common, m_data);

            // create new elements
            for (std::size_t i = common; i < other.m_size; ++i) {
                std::construct_at(m_data + i, other.m_data[i]);
            }

            // destroy extra elements
            std::destroy_n(m_data + other.m_size, m_size - common);

        } else {
            // destroy all elements
            clear();

            // prepare heap
            reserve_empty(other.m_size);

            std::uninitialized_copy_n(other.m_data, other.m_size, m_data);
        }

        m_size = other.m_size;
        return *this;
    }

    small_vector& operator=(small_vector&& other) {
        if (this == &other) {
            return *this;
        }

        // delete all elements
        clear();
        deallocate_heap();

        if (other.uses_stack()) {

            std::uninitialized_move_n(other.m_data, other.m_size, m_data);
            m_size = other.m_size;

            // already uses stack
            other.m_size = 0;

        } else {
            m_data     = other.m_data;
            m_size     = other.m_size;
            m_capacity = other.m_capacity;

            other.m_data     = other.m_stack.data();
            other.m_size     = 0;
            other.m_capacity = other.m_stack.size();
        }

        return *this;
    }

    ~small_vector() {
        clear();
        deallocate_heap();
    }

    /** @brief Removes all elements from the vector. */
    void clear() {
        if (!empty()) {
            std::destroy_n(m_data, m_size);
            m_size = 0;
        }
    }

    /** @brief Ensures that the vector can hold at least the requested size. */
    void reserve(std::size_t size) {
        if (size <= m_capacity) {
            return;
        }
        reallocate(size);
    }

    /**
     * @brief Changes the number of elements.
     *
     * @param size New number of elements.
     * @param value Value used to initialize new elements.
     */
    void resize(std::size_t size, const value_t& value = value_t { }) {
        if (m_size == size) {
            return;
        } else if (m_size > size) {
            std::destroy(m_data + size, m_data + m_size);

            m_size = size;
            return;
        }

        reserve(size);
        std::uninitialized_fill_n(m_data + m_size, size - m_size, value);

        m_size = size;
    }

    /** @brief Appends a copy of an element. */
    void push_back(const value_t& v) { emplace_back(v); }

    /** @brief Appends an element by moving it. */
    void push_back(value_t&& v) { emplace_back(std::move(v)); }

    /**
     * @brief Constructs an element at the end of the vector.
     *
     * @tparam Args Constructor argument types.
     * @param args Arguments forwarded to the element constructor.
     *
     * @return Reference to the newly constructed element.
     */
    template <typename... Args> value_t& emplace_back(Args&&... args) {
        if (m_size >= m_capacity) {
            std::size_t new_cap = new_capacity(m_size + 1);
            reallocate(new_cap);
        }

        alloc_traits::construct(m_alloc, m_data + m_size, std::forward<Args>(args)...);
        m_size += 1;
        return m_data[m_size - 1];
    }

    /** @brief Removes the last element. */
    void pop_back() {
        m_size -= 1;
        std::destroy_at(m_data + m_size);
    }

    /** @brief Returns whether the elements are stored in the stack buffer. */
    [[nodiscard]] bool uses_stack() const { return m_data == m_stack.data(); }

    /** @brief Returns whether the vector is empty. */
    [[nodiscard]] bool empty() const { return m_size == 0; }

    /** @brief Returns the number of elements. */
    [[nodiscard]] std::size_t size() const { return m_size; }

    /** @brief Returns the current capacity. */
    [[nodiscard]] std::size_t capacity() const { return m_capacity; }

    /** @brief Reduces capacity to fit the current size. */
    void shrink_to_fit() {
        if (uses_stack() || m_capacity == m_size) {
            return;
        }

        if (m_size <= m_stack.size()) {
            // move values to stack
            value_t* stack_ptr = m_stack.data();
            relocate(m_data, stack_ptr, m_size);

            // deallocate heap
            m_alloc.deallocate(m_data, m_capacity);

            m_data     = stack_ptr;
            m_capacity = m_stack.size();
        } else {
            reallocate(m_size);
        }
    }

    /** @brief Returns the element at the given index. */
    value_t& at(std::size_t i) { return m_data[i]; }

    /** @brief Returns the element at the given index. */
    const value_t& at(std::size_t i) const { return m_data[i]; }

    /** @brief Returns the element at the given index. */
    value_t& operator[](std::size_t i) { return at(i); }

    /** @brief Returns the element at the given index. */
    const value_t& operator[](std::size_t i) const { return at(i); }

    /** @brief Returns a pointer to the underlying element storage. */
    value_t* data() { return m_data; }

    /** @brief Returns a pointer to the underlying element storage. */
    const value_t* data() const { return m_data; }

    /** @brief Returns an iterator to the beginning of the small_vector. */
    iterator_t begin() { return m_data; }

    /** @brief Returns an iterator to the end of the small_vector. */
    iterator_t end() { return m_data + m_size; }

    /** @brief Returns an iterator to the beginning of the small_vector. */
    const_iterator_t begin() const { return m_data; }

    /** @brief Returns an iterator to the end of the small_vector. */
    const_iterator_t end() const { return m_data + m_size; }

private:
    stack_t m_stack;

    [[no_unique_address]] allocator_t m_alloc;

    std::size_t m_size;
    std::size_t m_capacity;
    value_t* m_data;

    /** @brief Ensures storage for the requested size without moving elements. */
    void reserve_empty(std::size_t size) {
        if (size <= m_capacity) {
            return;
        }
        reallocate_empty(size);
    }

    /** @brief Allocates storage without moving existing elements. */
    void reallocate_empty(std::size_t new_capacity) {
        value_t* new_data = m_alloc.allocate(new_capacity);

        if (!uses_stack()) {
            m_alloc.deallocate(m_data, m_capacity);
        }

        m_data     = new_data;
        m_capacity = new_capacity;
    }

    /** @brief Reallocates storage and moves existing elements. */
    void reallocate(std::size_t new_capacity) {
        value_t* new_data = m_alloc.allocate(new_capacity);
        relocate(m_data, new_data, m_size);

        if (!uses_stack()) {
            m_alloc.deallocate(m_data, m_capacity);
        }

        m_data     = new_data;
        m_capacity = new_capacity;
    }

    /** @brief Moves elements to new storage and destroys the old elements. */
    void relocate(value_t* data, value_t* new_data, std::size_t size) {
        std::uninitialized_move_n(data, size, new_data);
        std::destroy_n(data, size);
    }

    /** @brief Releases heap storage and restores stack storage. */
    void deallocate_heap() {
        if (!uses_stack() && m_data != nullptr) {
            alloc_traits::deallocate(m_alloc, m_data, m_capacity);
        }

        m_data     = m_stack.data();
        m_capacity = m_stack.size();
    }

    /** @brief Calculates the next capacity for growth. */
    std::size_t new_capacity(std::size_t requested) {
        std::size_t c = m_capacity + (m_capacity / 2);

        if (c < requested) {
            return requested;
        } else {
            return c;
        }
    }
};

}

#endif
