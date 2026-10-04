#ifndef KOUTIL_CONTAINER_INDEX_H
#define KOUTIL_CONTAINER_INDEX_H

#include <concepts>
#include <type_traits>

namespace koutil::container {

/**
 * @brief Wrapper around the index tag
 */
template <typename T> struct type_tag {
    using type = T;
};

/**
 * @brief Represents a compile-time index value in an index conversion rule.
 *
 * @tparam Value The compile-time value represented by this rule operand.
 *
 * @code
 * koutil::container::index_value<IndexTag::A>
 * @endcode
 */
template <auto Value> struct index_value {
    using type                  = decltype(Value);
    static constexpr type value = Value;
};

/**
 * @brief Represents a type in an index conversion rule.
 *
 * @tparam T The type represented by this rule operand.
 *
 * @code
 * koutil::container::index_type<float>
 * @endcode
 */
template <typename T> struct index_type {
    using type = T;
};

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

namespace detail {

    // default rule
    constexpr bool allow_index_conversion(...) { return false; }

    template <typename From, typename To> consteval bool check_index_conversion() {
        return allow_index_conversion(type_tag<From> { }, type_tag<To> { });
    }

    template <typename T> struct is_index_tag : std::false_type { };

    template <typename TagType, TagType Value> struct is_index_tag<index_tag<TagType, Value>> : std::true_type { };

    //-- Rules -----------------------------------------------------------------

    template <typename T, typename Arg> struct rule_match : std::false_type { };

    // types
    template <typename T, typename Expected>
    struct rule_match<T, index_type<Expected>> : std::bool_constant<std::is_same_v<T, Expected>> { };

    // values
    template <typename T, auto Expected>
        requires(is_index_tag<T>::value)
    struct rule_match<T, index_value<Expected>>
        : std::bool_constant<std::is_same_v<decltype(Expected), typename T::tag_t> && Expected == T::value> { };

    template <typename T, typename Rule> inline constexpr bool rule_match_v = rule_match<T, Rule>::value;

}

/**
 * @brief Defines a single allowed index conversion rule.
 *
 * The rule can describe conversions between types, compile-time values,
 * or a combination of both.
 *
 * @tparam From The source of the conversion rule.
 * @tparam To The destination of the conversion rule.
 *
 * @code
 * using rule_a = index_rule<
 *     index_value<IndexTag::A>,
 *     index_value<IndexTag::B>
 * >;
 *
 * using rule_b = index_rule<
 *     index_value<IndexTag::C>,
 *     index_type<float>
 * >;
 * @endcode
 */
template <typename From, typename To> struct index_rule {
    template <typename F, typename T> static consteval bool matches() {
        return detail::rule_match_v<F, From> && detail::rule_match_v<T, To>;
    }
};

/**
 * @brief Collection of index conversion rules.
 *
 * A conversion is allowed when at least one of the contained rules matches
 * the source and destination.
 *
 * @tparam Rules The conversion rules.
 *
 * @code
 * using conversion_rules = index_rules<
 *     KOUTIL_INDEX_RULE_VALUE(IndexTag::A, IndexTag::B),
 *     KOUTIL_INDEX_RULE_VALUE(IndexTag::B, IndexTag::C),
 *     KOUTIL_INDEX_RULE_VALUE_TYPE(IndexTag::C, float),
 *     KOUTIL_INDEX_RULE_TYPE_VALUE(float, IndexTag::A)
 * >;
 * @endcode
 */
template <typename... Rules> struct index_rules {
    template <typename From, typename To> static consteval bool allows() {
        return (Rules::template matches<From, To>() || ...);
    }
};

/**
 * @brief Concept identifying an index tag type.
 * @tparam T The type to check.
 */
template <typename T>
concept index_tag_type = detail::is_index_tag<T>::value;

/**
 * @brief Convenience variable for @ref allow_index_conversion.
 *
 * @tparam From Source index tag.
 * @tparam To   Destination index tag.
 */
template <typename From, typename To>
inline constexpr bool allow_index_conversion_v = detail::check_index_conversion<From, To>();

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
 * @brief Creates an index conversion rule between two compile-time values.
 *
 * This macro creates a rule allowing conversion from one index value to
 * another index value.
 *
 * @param FROM The source index value.
 * @param TO The destination index value.
 *
 * @code
 * using rules = koutil::container::index_rules<
 *     KOUTIL_INDEX_RULE_VALUE(IndexTag::A, IndexTag::B),
 *     KOUTIL_INDEX_RULE_VALUE(IndexTag::B, IndexTag::C)
 * >;
 * @endcode
 */
#define KOUTIL_INDEX_RULE_VALUE(FROM, TO) \
    ::koutil::container::index_rule<::koutil::container::index_value<FROM>, ::koutil::container::index_value<TO>>

/**
 * @brief Creates an index conversion rule between two types.
 *
 * This macro creates a rule allowing conversion from one index type to
 * another index type.
 *
 * @param FROM The source index type.
 * @param TO The destination index type.
 *
 * @code
 * using rules = koutil::container::index_rules<
 *     KOUTIL_INDEX_RULE_TYPE(MyIndex, float),
 *     KOUTIL_INDEX_RULE_TYPE(float, MyIndex)
 * >;
 * @endcode
 */
#define KOUTIL_INDEX_RULE_TYPE(FROM, TO) \
    ::koutil::container::index_rule<::koutil::container::index_type<FROM>, ::koutil::container::index_type<TO>>

/**
 * @brief Creates an index conversion rule from a type to a compile-time value.
 *
 * This macro creates a rule allowing conversion from an index type to an
 * index value.
 *
 * @param FROM The source index type.
 * @param TO The destination index value.
 *
 * @code
 * using rules = koutil::container::index_rules<
 *     KOUTIL_INDEX_RULE_TYPE_VALUE(float, IndexTag::A)
 * >;
 * @endcode
 */
#define KOUTIL_INDEX_RULE_TYPE_VALUE(FROM, TO) \
    ::koutil::container::index_rule<::koutil::container::index_type<FROM>, ::koutil::container::index_value<TO>>

/**
 * @brief Creates an index conversion rule from a compile-time value to a type.
 *
 * This macro creates a rule allowing conversion from an index value to an
 * index type.
 *
 * @param FROM The source index value.
 * @param TO The destination index type.
 *
 * @code
 * using rules = koutil::container::index_rules<
 *     KOUTIL_INDEX_RULE_VALUE_TYPE(IndexTag::C, float)
 * >;
 * @endcode
 */
#define KOUTIL_INDEX_RULE_VALUE_TYPE(FROM, TO) \
    ::koutil::container::index_rule<::koutil::container::index_value<FROM>, ::koutil::container::index_type<TO>>

}

#endif
