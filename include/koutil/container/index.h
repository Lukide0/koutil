#ifndef KOUTIL_CONTAINER_INDEX_H
#define KOUTIL_CONTAINER_INDEX_H

#include <concepts>
#include <type_traits>

namespace koutil::container {

/**
 * @brief Compile-time tag identified by a type and value.
 *
 * @tparam TagType Type of the tag value.
 * @tparam Value   Compile-time tag value.
 *
 * @par Example
 * @code
 * using user_tag = index_tag<IndexKind, IndexKind::Meta>;
 * @endcode
 */
template <typename TagType, TagType Value> struct index_tag {
    using tag_t                  = TagType;
    static constexpr tag_t value = Value;
};

/**
 * @brief Customization point for conversions between index tags.
 *
 * @tparam From Source tag value.
 * @tparam To   Destination tag value.
 *
 * @par Example
 * @code
 * template <>
 * struct allow_index_tag_conversion<IndexKind::Meta, IndexKind::Data>
 *     : std::true_type {};
 * @endcode
 */
template <auto From, auto To> struct allow_index_tag_conversion : std::false_type { };

/**
 * @brief Customization point for conversions between index types.
 *
 * Conversions are disabled by default.
 *
 * @tparam From Source index tag.
 * @tparam To   Destination index tag.
 */
template <typename From, typename To> struct allow_index_conversion : std::false_type { };

/**
 * @brief Enables conversions between @ref index_tag types.
 *
 * @tparam FromType Type of the source tag.
 * @tparam FromTag  Source tag value.
 * @tparam ToType   Type of the destination tag.
 * @tparam ToTag    Destination tag value.
 */
template <typename FromType, FromType FromTag, typename ToType, ToType ToTag>
struct allow_index_conversion<index_tag<FromType, FromTag>, index_tag<ToType, ToTag>>
    : allow_index_tag_conversion<FromTag, ToTag> { };

/**
 * @brief Convenience variable for @ref allow_index_conversion.
 *
 * @tparam From Source index tag.
 * @tparam To   Destination index tag.
 */
template <typename From, typename To>
inline constexpr bool allow_index_conversion_v = allow_index_conversion<From, To>::value;

/**
 * @brief Strongly typed index value.
 *
 * Conversions between different tags are disabled by default and must be
 * explicitly enabled through @ref allow_index_conversion.
 *
 * @tparam ValueType Type used to store the index value.
 * @tparam Tag       Tag identifying the index type.
 *
 * @par Example
 * @code
 * using player_index = index_t<std::uint32_t, PlayerTag>;
 * using enemy_index = index_t<std::uint32_t, EnemyTag>;
 *
 * player_index user{5};
 * enemy_index func{user}; // Only valid if the conversion is allowed.
 * @endcode
 */
template <typename ValueType, typename Tag> class index_t {
public:
    using value_t = ValueType;
    using tag_t   = Tag;

    constexpr index_t()               = default;
    constexpr index_t(const index_t&) = default;
    constexpr index_t(index_t&&)      = default;

    /**
     * @brief Constructs an index from another tag when the conversion is allowed.
     *
     * @tparam OtherTag Source index tag.
     * @param other Source index.
     */
    template <typename OtherTag>
        requires(allow_index_conversion_v<OtherTag, Tag>)
    constexpr index_t(const index_t<ValueType, OtherTag>& other)
        : m_value(other.value()) { }

    template <typename OtherTag>
        requires(!allow_index_conversion_v<OtherTag, Tag>)
    constexpr index_t(const index_t<ValueType, OtherTag>& other) = delete;

    /**
     * @brief Constructs an index from a convertible value.
     *
     * @param value Value used to initialize the index.
     */
    constexpr explicit index_t(std::convertible_to<value_t> auto value)
        : m_value(value) { }

    index_t& operator=(index_t&&)      = default;
    index_t& operator=(const index_t&) = default;

    [[nodiscard]] constexpr value_t value() const { return m_value; }

    [[nodiscard]] constexpr operator value_t() const { return m_value; }

private:
    value_t m_value;
};

/**
 * @brief Creates an @ref index_t using an @ref index_tag.
 *
 * @tparam ValueType Type used to store the index value.
 * @tparam TagValue  Compile-time tag value.
 *
 * @par Example
 * @code
 * using player_index = index_tagged_t<std::uint32_t, IndexKind::Player>;
 * using enemy_index = index_tagged_t<std::uint32_t, IndexKind::Enemy>;
 * @endcode
 */
template <typename ValueType, auto TagValue>
using index_tagged_t = index_t<ValueType, index_tag<decltype(TagValue), TagValue>>;

/**
 * @def KOUTIL_ALLOW_INDEX_TAG_CONV
 * @brief Allows conversion between two tagged index values.
 *
 * @param FROM Source tag value.
 * @param TO   Destination tag value.
 *
 * @par Example
 * @code
 * KOUTIL_ALLOW_INDEX_TAG_CONVERSION(IndexKind::Meta, IndexKind::Data);
 * @endcode
 */
#define KOUTIL_ALLOW_INDEX_TAG_CONV(FROM, TO) \
    template <> struct ::koutil::container::allow_index_tag_conversion<FROM, TO> : std::true_type { }

/**
 * @def KOUTIL_ALLOW_INDEX_CONV
 * @brief Allows conversion between two index tag types.
 *
 * @param FROM Source index tag type.
 * @param TO   Destination index tag type.
 *
 * @par Example
 * @code
 * KOUTIL_ALLOW_INDEX_CONVERSION(FooTag, BarTag);
 * @endcode
 */
#define KOUTIL_ALLOW_INDEX_CONV(FROM, TO) \
    template <> struct ::koutil::container::allow_index_conversion<FROM, TO> : std::true_type { }

/**
 * @def KOUTIL_ALLOW_INDEX_TO_TAG_CONV
 * @brief Allows conversion from an untagged index to a tagged index.
 *
 * @param FROM Source index tag type.
 * @param TO   Destination tag value.
 *
 * @par Example
 * @code
 * KOUTIL_ALLOW_INDEX_TO_TAG_CONV(float, IndexKind::Meta);
 * @endcode
 */
#define KOUTIL_ALLOW_INDEX_TO_TAG_CONV(FROM, TO)                                                               \
    template <>                                                                                                \
    struct ::koutil::container::allow_index_conversion<FROM, ::koutil::container::index_tag<decltype(TO), TO>> \
        : std::true_type { }

/**
 * @def KOUTIL_ALLOW_INDEX_FROM_TAG_CONV
 * @brief Allows conversion from a tagged index to an untagged index.
 *
 * @param FROM Source tag value.
 * @param TO   Destination index tag type.
 *
 * @par Example
 * @code
 * KOUTIL_ALLOW_INDEX_FROM_TAG_CONV(IndexKind::Meta, float);
 * @endcode
 */
#define KOUTIL_ALLOW_INDEX_FROM_TAG_CONV(FROM, TO)                                                               \
    template <>                                                                                                  \
    struct ::koutil::container::allow_index_conversion<::koutil::container::index_tag<decltype(FROM), FROM>, TO> \
        : std::true_type { }

}

#endif
