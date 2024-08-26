#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <doctest/doctest.h>
#include <koutil/container/multi_vector.h>
#include <tuple>

using namespace koutil::container;

constexpr std::size_t INIT_SIZE   = 5;
constexpr std::size_t INSERT_SIZE = 5;

TEST_CASE("[MULTI_VECTOR][CONSTRUCTORS]") {
    {
        multi_vector<int, int> vec_2d;
        REQUIRE_EQ(vec_2d.size(), 0);

        vec_2d.push_back(std::make_tuple(0, 0));
        vec_2d.push_back(std::make_tuple(0, 1));
        vec_2d.push_back(std::make_tuple(0, 2));

        REQUIRE_EQ(vec_2d.size(), 3);

        multi_vector<int, int> vec_2d_copy = vec_2d;
        CHECK_EQ(vec_2d_copy.size(), 3);

        multi_vector<int, int> vec_2d_move = std::move(vec_2d);
        CHECK_EQ(vec_2d_move.size(), 3);
    }

    SUBCASE("[MULTI_VECTOR][CONSTRUCTOR][DEFAULT_INIT]") {
        multi_vector<int, int> vec_2d(INIT_SIZE);
        CHECK_EQ(vec_2d.size(), INIT_SIZE);
    }

    SUBCASE("[MULTI_VECTOR][CONSTRUCTOR][INIT]") {
        multi_vector<int, int> vec_2d(INIT_SIZE, std::make_tuple(5, 5));
        REQUIRE_EQ(vec_2d.size(), INIT_SIZE);

        for (std::size_t i = 0; i < INIT_SIZE; ++i) {

            const auto& value = vec_2d[i];
            CHECK_EQ(value, std::make_tuple(5, 5));
        }
    }
}

TEST_CASE("[MULTI_VECTOR][PUSH_BACK]") {
    multi_vector<int, int> vec_2d;
    REQUIRE_EQ(vec_2d.size(), 0);

    for (std::size_t i = 0; i < INSERT_SIZE; ++i) {
        vec_2d.push_back(std::make_tuple(1, 2));
        REQUIRE_EQ(vec_2d.size(), i + 1);
    }
}

TEST_CASE("[MULTI_VECTOR][EMPLACE_BACK]") {
    multi_vector<int, int> vec_2d;
    REQUIRE_EQ(vec_2d.size(), 0);

    for (std::size_t i = 0; i < INSERT_SIZE; ++i) {
        vec_2d.emplace_back(1, 2);
        REQUIRE_EQ(vec_2d.size(), i + 1);
    }
}

TEST_CASE("[MULTI_VECTOR][CLEAR]") {
    multi_vector<int, int, int> vec_3d(INIT_SIZE);
    REQUIRE_EQ(vec_3d.size(), INIT_SIZE);

    vec_3d.clear();
    CHECK_EQ(vec_3d.size(), 0);
}

TEST_CASE("[MULTI_VECTOR][SWAP]") {

    multi_vector<int, int> vec_2d(INIT_SIZE);
    REQUIRE_EQ(vec_2d.size(), INIT_SIZE);

    multi_vector<int, int> vec_2d_empty;
    REQUIRE_EQ(vec_2d_empty.size(), 0);

    vec_2d.swap(vec_2d_empty);
    CHECK_EQ(vec_2d.size(), 0);
    CHECK_EQ(vec_2d_empty.size(), INIT_SIZE);

    vec_2d.swap(vec_2d_empty);
    CHECK_EQ(vec_2d.size(), INIT_SIZE);
    CHECK_EQ(vec_2d_empty.size(), 0);
}

TEST_CASE("[MULTI_VECTOR][RESIZE]") {
    multi_vector<int, int> vec_2d;
    REQUIRE_EQ(vec_2d.size(), 0);

    vec_2d.resize(INIT_SIZE);
    CHECK_EQ(vec_2d.size(), INIT_SIZE);

    vec_2d.clear();
    REQUIRE_EQ(vec_2d.size(), 0);

    vec_2d.emplace_back(5, 5);
    REQUIRE_EQ(vec_2d.size(), 1);

    vec_2d.resize(INIT_SIZE, std::make_tuple(6, 6));
    CHECK_EQ(vec_2d[0], std::make_tuple(5, 5));

    for (std::size_t i = 1; i < INIT_SIZE; ++i) {
        CHECK_EQ(vec_2d[i], std::make_tuple(6, 6));
    }
}

TEST_CASE("[MULTI_VECTOR][POP_BACK]") {
    multi_vector<int, int> vec_2d(INIT_SIZE);
    REQUIRE_EQ(vec_2d.size(), INIT_SIZE);

    for (std::size_t i = 0; i < INIT_SIZE; ++i) {
        vec_2d.pop_back();
        CHECK_EQ(vec_2d.size(), INIT_SIZE - 1 - i);
    }
}

TEST_CASE("[MULTI_VECTOR][AT]") {
    multi_vector<int, int> vec(INIT_SIZE);
    REQUIRE_EQ(vec.size(), INIT_SIZE);

    for (int i = 0; i < static_cast<int>(INIT_SIZE); ++i) {
        auto val = vec[i];

        std::get<0>(val) = i;
        std::get<1>(val) = i * 2;

        CHECK_EQ(vec[i], std::make_tuple(i, i * 2));
    }
}

TEST_CASE("[MULTI_VECTOR][ERASE]" * doctest::timeout(10)) {
    multi_vector<int, int> vec(INIT_SIZE);
    REQUIRE_EQ(vec.size(), INIT_SIZE);

    while (!vec.empty()) {
        vec.erase(vec.begin());
    }

    CHECK_EQ(vec.size(), 0);
}

TEST_CASE("[MULTI_VECTOR][ITERATOR]") {
    multi_vector<int, int> vec;

    for (std::size_t i = 0; i < INIT_SIZE; ++i) {
        vec.emplace_back(i, i);
    }
    REQUIRE_EQ(vec.size(), INIT_SIZE);

    std::size_t i = 0;
    for (auto&& [a, b] : vec) {

        CHECK_EQ(a, i);
        CHECK_EQ(b, i);

        i += 1;
    }

    // NOLINTNEXTLINE
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        auto&& [a, b] = *it;

        a = a * 2;
        b = b * 3;
    }

    i = 0;
    for (auto&& [a, b] : vec) {
        CHECK_EQ(a, i * 2);
        CHECK_EQ(b, i * 3);

        i += 1;
    }
}
