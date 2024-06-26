#include "koutil/container/hash_array.h"
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <doctest/doctest.h>
#include <functional>
#include <koutil/container/multi_vector.h>
#include <tuple>
#include <utility>
#include <vector>

using namespace koutil::container;

struct CustomKey {
    int a;
    int b;

    bool operator==(const CustomKey&) const = default;
};

struct KeyAdapter {
    KeyAdapter(std::vector<int>& storage)
        : container(storage) { }

    [[nodiscard]] bool eql(const CustomKey& key, std::size_t key_index) const {
        return key.a == container[key_index] && key.b == container[key_index + 1];
    }

    std::vector<int>& container;
};

struct HashKey {
    std::size_t operator()(const CustomKey& key) {
        auto value = (static_cast<std::uint64_t>(key.a) << 32) + key.b;

        return std::hash<std::uint64_t>()(value);
    }
};

std::size_t insert_key(const CustomKey& key, std::vector<int>& storage) {
    storage.push_back(key.a);
    storage.push_back(key.b);

    return storage.size() - 2;
}

using hash_array_t = hash_array<CustomKey, std::size_t, KeyAdapter, HashKey>;

TEST_CASE("[HASH_ARRAY][CONSTRUCTORS]") {

    std::vector<int> storage;

    CustomKey a { 1, 2 };
    CustomKey b { 2, 4 };
    CustomKey c { 3, 6 };

    auto a_index = insert_key(a, storage);
    auto b_index = insert_key(b, storage);
    auto c_index = insert_key(c, storage);

    KeyAdapter adapter { storage };

    SUBCASE("[HASH_ARRAY][CONSTRUCTOR][DEFAULT_INIT]") {
        hash_array_t array;
        CHECK(array.try_insert(a, a_index, adapter));
        CHECK(array.try_insert(b, b_index, adapter));
        CHECK(array.try_insert(c, c_index, adapter));

        CHECK_EQ(array.size(), 3);

        array.clear();

        CHECK_EQ(array.size(), 0);
    }

    SUBCASE("[HASH_ARRAY][CONSTRUCTOR][INIT]") {
        hash_array_t array(4);
        CHECK_EQ(array.bucket_count(), 4);
        CHECK_EQ(array.size(), 0);
    }

    SUBCASE("[HASH_ARRAY][CONSTRUCTOR][COPY]") {
        hash_array_t a_array;
        CHECK(a_array.try_insert(a, a_index, adapter));
        CHECK(a_array.try_insert(b, b_index, adapter));

        REQUIRE_EQ(a_array.size(), 2);

        hash_array_t b_array = a_array;

        CHECK(b_array.try_insert(c, c_index, adapter));

        REQUIRE_EQ(b_array.size(), 3);

        b_array = a_array;

        CHECK_EQ(b_array.size(), 2);
    }

    SUBCASE("[HASH_ARRAY][CONSTRUCTOR][MOVE]") {
        hash_array_t a_array;
        CHECK(a_array.try_insert(a, a_index, adapter));
        CHECK(a_array.try_insert(b, b_index, adapter));

        REQUIRE_EQ(a_array.size(), 2);

        hash_array_t b_array = std::move(a_array);

        CHECK_EQ(b_array.size(), 2);
    }
}

TEST_CASE("[HASH_ARRAY][TRY]") {

    std::vector<int> storage;

    CustomKey a { 1, 2 };
    CustomKey b { 2, 4 };
    CustomKey c { 3, 6 };
    CustomKey d { 2, 4 };

    auto a_index = insert_key(a, storage);
    auto b_index = insert_key(b, storage);
    auto c_index = insert_key(c, storage);
    auto d_index = insert_key(d, storage);

    CHECK_NE(b_index, d_index);
    CHECK_EQ(b, d);

    KeyAdapter adapter { storage };

    hash_array_t array;
    CHECK(array.try_insert(a, a_index, adapter));
    CHECK(array.try_insert(b, b_index, adapter));
    CHECK(array.try_insert(c, c_index, adapter));
    CHECK(array.try_insert(c, c_index, adapter) == false);

    CHECK(array.try_set(CustomKey { 50, 90 }, 5, adapter) == false);
    CHECK(array.try_set(b, d_index, adapter) == true);
}

TEST_CASE("[HASH_ARRAY][FIND]") {

    std::vector<int> storage;

    CustomKey a { 1, 2 };
    CustomKey b { 2, 4 };
    CustomKey c { 3, 6 };

    auto a_index = insert_key(a, storage);
    auto b_index = insert_key(b, storage);
    auto c_index = insert_key(c, storage);

    KeyAdapter adapter { storage };

    hash_array_t array;
    CHECK(array.try_insert(a, a_index, adapter));
    CHECK(array.try_insert(b, b_index, adapter));
    CHECK(array.try_insert(c, c_index, adapter));

    auto it = array.find(b, adapter);

    CHECK_NE(it, array.end());
    CHECK_EQ(*it, b_index);

    it = array.find({ 850, 80 }, adapter);
    CHECK_EQ(it, array.end());
}

TEST_CASE("[HASH_ARRAY][ERASE]") {

    std::vector<int> storage;

    CustomKey a { 1, 2 };
    CustomKey b { 2, 4 };
    CustomKey c { 3, 6 };

    auto a_index = insert_key(a, storage);
    auto b_index = insert_key(b, storage);
    auto c_index = insert_key(c, storage);

    KeyAdapter adapter { storage };

    hash_array_t array;
    CHECK(array.try_insert(a, a_index, adapter));
    CHECK(array.try_insert(b, b_index, adapter));
    CHECK(array.try_insert(c, c_index, adapter));

    array.erase(a, adapter);
    array.erase(b, adapter);
    array.erase(c, adapter);

    CHECK(array.empty());
}

TEST_CASE("[HASH_ARRAY][ITERATOR]") {

    std::vector<int> storage;

    CustomKey a { 1, 2 };
    CustomKey b { 2, 4 };
    CustomKey c { 3, 6 };

    auto a_index = insert_key(a, storage);
    auto b_index = insert_key(b, storage);
    auto c_index = insert_key(c, storage);

    KeyAdapter adapter { storage };

    hash_array_t array;
    CHECK(array.try_insert(a, a_index, adapter));
    CHECK(array.try_insert(b, b_index, adapter));
    CHECK(array.try_insert(c, c_index, adapter));

    std::size_t find = 0;
    for (auto&& index : array) {
        if (index == a_index) {
            find |= 0b100;
        } else if (index == b_index) {
            find |= 0b010;
        } else {
            CHECK_EQ(index, c_index);
            find |= 0b001;
        }
    }

    CHECK_EQ(find, 0b111);
}
