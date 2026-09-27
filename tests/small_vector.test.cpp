#include <cstdlib>
#include <doctest/doctest.h>
#include <koutil/container/small_vector.h>
#include <utility>

using namespace koutil::container;

constexpr std::size_t STACK_SIZE  = 5;
constexpr std::size_t INIT_SIZE   = 5;
constexpr std::size_t INSERT_SIZE = 5;

TEST_CASE("[SMALL_VECTOR][CONSTRUCTORS]") {
    {
        small_vector<int, STACK_SIZE> vec;
        REQUIRE_EQ(vec.size(), 0);

        vec.push_back(0);
        vec.push_back(1);
        vec.push_back(2);

        REQUIRE_EQ(vec.size(), 3);

        small_vector<int, STACK_SIZE> vec_copy = vec;
        CHECK_EQ(vec_copy.size(), 3);

        small_vector<int, STACK_SIZE> vec_move = std::move(vec);
        CHECK_EQ(vec_move.size(), 3);
    }

    SUBCASE("[SMALL_VECTOR][CONSTRUCTOR][DEFAULT_INIT]") {
        small_vector<int, STACK_SIZE> vec(INIT_SIZE);
        CHECK_EQ(vec.size(), INIT_SIZE);
    }

    SUBCASE("[SMALL_VECTOR][CONSTRUCTOR][INIT]") {
        small_vector<int, STACK_SIZE> vec(INIT_SIZE, 5);
        REQUIRE_EQ(vec.size(), INIT_SIZE);

        for (std::size_t i = 0; i < INIT_SIZE; ++i) {

            const auto& value = vec[i];
            CHECK_EQ(value, 5);
        }
    }
}

TEST_CASE("[SMALL_VECTOR][PUSH_BACK]") {
    small_vector<int, STACK_SIZE> vec;
    REQUIRE_EQ(vec.size(), 0);

    for (std::size_t i = 0; i < INSERT_SIZE; ++i) {
        vec.push_back(i);
        REQUIRE_EQ(vec.size(), i + 1);
    }

    for (std::size_t i = 0; i < INSERT_SIZE; ++i) {
        REQUIRE_EQ(vec.at(i), i);
    }
}

TEST_CASE("[SMALL_VECTOR][EMPLACE_BACK]") {
    struct pair {
        int x;
        int y;

        pair(int a, int b)
            : x(a)
            , y(b) { }
    };

    small_vector<pair, STACK_SIZE> vec;
    REQUIRE_EQ(vec.size(), 0);

    for (std::size_t i = 0; i < INSERT_SIZE; ++i) {
        vec.emplace_back(i, i * 2);
        REQUIRE_EQ(vec.size(), i + 1);
    }

    for (std::size_t i = 0; i < INSERT_SIZE; ++i) {
        const auto& p = vec.at(i);

        REQUIRE_EQ(p.x, i);
        REQUIRE_EQ(p.y, i * 2);
    }
}

TEST_CASE("[SMALL_VECTOR][CLEAR]") {
    small_vector<int, STACK_SIZE> vec(INIT_SIZE);
    REQUIRE_EQ(vec.size(), INIT_SIZE);

    vec.clear();
    CHECK_EQ(vec.size(), 0);
}

TEST_CASE("[SMALL_VECTOR][RESIZE]") {
    small_vector<int, STACK_SIZE> vec;
    REQUIRE_EQ(vec.size(), 0);

    vec.resize(INIT_SIZE);
    CHECK_EQ(vec.size(), INIT_SIZE);

    vec.clear();
    REQUIRE_EQ(vec.size(), 0);

    vec.emplace_back(5);
    REQUIRE_EQ(vec.size(), 1);

    vec.resize(INIT_SIZE, 6);
    CHECK_EQ(vec[0], 5);

    for (std::size_t i = 1; i < INIT_SIZE; ++i) {
        CHECK_EQ(vec[i], 6);
    }
}

TEST_CASE("[SMALL_VECTOR][POP_BACK]") {
    small_vector<int, STACK_SIZE> vec(INIT_SIZE);
    REQUIRE_EQ(vec.size(), INIT_SIZE);

    for (std::size_t i = 0; i < INIT_SIZE; ++i) {
        vec.pop_back();
        CHECK_EQ(vec.size(), INIT_SIZE - 1 - i);
    }

    small_vector<int, 0> vec_zero(INIT_SIZE);
    REQUIRE_EQ(vec_zero.size(), INIT_SIZE);

    for (std::size_t i = 0; i < INIT_SIZE; ++i) {
        vec_zero.pop_back();
        CHECK_EQ(vec_zero.size(), INIT_SIZE - 1 - i);
    }
}

TEST_CASE("[SMALL_VECTOR][USES_STACK]") {
    small_vector<int, STACK_SIZE> vec;
    REQUIRE_EQ(vec.size(), 0);

    for (std::size_t i = 0; i < STACK_SIZE; ++i) {
        vec.push_back(i);
        REQUIRE(vec.uses_stack());
    }

    for (std::size_t i = 0; i < STACK_SIZE; ++i) {
        vec.push_back(i);
        REQUIRE(!vec.uses_stack());
    }

    for (std::size_t i = 0; i < STACK_SIZE * 2; ++i) {
        vec.pop_back();
        REQUIRE(!vec.uses_stack());
    }

    REQUIRE_EQ(vec.size(), 0);
}

TEST_CASE("[SMALL_VECTOR][SHRINK_TO_FIT]") {
    small_vector<int, STACK_SIZE> vec(STACK_SIZE * 2);
    REQUIRE_EQ(vec.size(), STACK_SIZE * 2);
    REQUIRE(!vec.uses_stack());

    for (std::size_t i = 0; i < STACK_SIZE; ++i) {
        vec.pop_back();
    }

    REQUIRE(!vec.uses_stack());
    vec.shrink_to_fit();
    REQUIRE(vec.uses_stack());
}

TEST_CASE("[SMALL_VECTOR][ITERATOR]") {
    small_vector<int, STACK_SIZE> vec;

    for (std::size_t i = 0; i < INIT_SIZE; ++i) {
        vec.emplace_back(i);
    }
    REQUIRE_EQ(vec.size(), INIT_SIZE);

    std::size_t i = 0;
    for (auto&& a : vec) {
        CHECK_EQ(a, i);
        i += 1;
    }

    // NOLINTNEXTLINE
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        auto&& a = *it;

        a = a * 2;
    }

    i = 0;
    for (auto&& a : vec) {
        CHECK_EQ(a, i * 2);
        i += 1;
    }
}
