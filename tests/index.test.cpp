#include <concepts>
#include <cstdint>
#include <doctest/doctest.h>
#include <koutil/container/index.h>
#include <type_traits>

using namespace koutil::container;

enum class IndexTag {
    A,
    B,
    C,
};

/*
Conversions:
A -> B
B -> C
C -> D
D -> A
*/

KOUTIL_ALLOW_INDEX_TAG_CONV(IndexTag::A, IndexTag::B);
KOUTIL_ALLOW_INDEX_TAG_CONV(IndexTag::B, IndexTag::C);
KOUTIL_ALLOW_INDEX_FROM_TAG_CONV(IndexTag::C, float);
KOUTIL_ALLOW_INDEX_TO_TAG_CONV(float, IndexTag::A);

using A = index_tagged_t<std::uint32_t, IndexTag::A>;
using B = index_tagged_t<std::uint32_t, IndexTag::B>;
using C = index_tagged_t<std::uint32_t, IndexTag::C>;
using D = index_t<std::uint32_t, float>;

TEST_CASE("[INDEX]") {

    // default constructor
    static_assert(std::constructible_from<A, std::uint32_t>);
    static_assert(std::constructible_from<B, std::uint32_t>);
    static_assert(std::constructible_from<C, std::uint32_t>);
    static_assert(std::constructible_from<D, std::uint32_t>);

    // Only A -> A and A -> B
    static_assert(std::constructible_from<A, A>);
    static_assert(std::constructible_from<B, A>);
    static_assert(!std::constructible_from<C, A>);
    static_assert(!std::constructible_from<D, A>);

    // Only B -> B and B -> C
    static_assert(!std::constructible_from<A, B>);
    static_assert(std::constructible_from<B, B>);
    static_assert(std::constructible_from<C, B>);
    static_assert(!std::constructible_from<D, B>);

    // Only C -> C and C -> D
    static_assert(!std::constructible_from<A, C>);
    static_assert(!std::constructible_from<B, C>);
    static_assert(std::constructible_from<C, C>);
    static_assert(std::constructible_from<D, C>);

    // Only D -> D and D -> A
    static_assert(std::constructible_from<A, D>);
    static_assert(!std::constructible_from<B, D>);
    static_assert(!std::constructible_from<C, D>);
    static_assert(std::constructible_from<D, D>);
}
