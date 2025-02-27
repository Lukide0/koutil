#include "koutil/container/hash_array.h"
#include "koutil/container/template_hash_array.h"
#include <cassert>
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

enum class CustomKeyTag {
    ONE,
    TWO,
    THREE,
};

struct CustomKey {
    int a;
    int b;

    bool operator==(const CustomKey&) const = default;
};

struct CustomKeyTemplate {
    std::uint16_t a;
    std::uint16_t b;
    CustomKeyTag tag;

    bool operator==(const CustomKeyTemplate&) const = default;
};

struct KeyAdapter {
    KeyAdapter(std::vector<int>& storage)
        : container(storage) { }

    [[nodiscard]] bool eql(const CustomKey& key, std::size_t key_index) const {
        return key.a == container[key_index * 2] && key.b == container[(key_index * 2) + 1];
    }

    std::vector<int>& container;
};

struct KeyAdapterTemplate {
    KeyAdapterTemplate(std::vector<int>& storage, std::vector<CustomKeyTag>& tags)
        : container(storage)
        , tags_container(tags) { }

    template <CustomKeyTag Tag> [[nodiscard]] bool eql(const CustomKeyTemplate& key, std::size_t key_index) const {
        return tags_container[key_index] == Tag && key.a == container[key_index * 2]
            && key.b == container[(key_index * 2) + 1];
    }

    std::vector<int>& container;
    std::vector<CustomKeyTag>& tags_container;
};

struct HashKey {
    std::size_t operator()(const CustomKey& key) {
        auto value = (static_cast<std::uint64_t>(key.a) << 32) + key.b;

        return std::hash<std::uint64_t>()(value);
    }
};

struct HashKeyTemplate {
    template <CustomKeyTag Tag> [[nodiscard]] std::size_t hash(const CustomKeyTemplate& key) const {

        auto value = key.a + (static_cast<std::uint64_t>(key.b) << (sizeof(key.a) * 8))
            + (static_cast<std::uint64_t>(key.tag) << ((sizeof(key.a) + sizeof(key.b)) * 8));

        return std::hash<std::uint64_t>()(value);
    }
};

std::size_t insert_key(const CustomKey& key, std::vector<int>& storage) {
    storage.push_back(key.a);
    storage.push_back(key.b);

    return (storage.size() / 2) - 1;
}

std::size_t insert_key(const CustomKeyTemplate& key, std::vector<int>& storage, std::vector<CustomKeyTag>& tags) {
    storage.push_back(key.a);
    storage.push_back(key.b);

    tags.push_back(key.tag);

    assert(storage.size() / 2 == tags.size());

    return (storage.size() / 2) - 1;
}

using hash_array_t = hash_array<CustomKey, std::size_t, KeyAdapter, HashKey>;
using template_hash_array_t
    = template_hash_array<CustomKeyTemplate, std::size_t, CustomKeyTag, KeyAdapterTemplate, HashKeyTemplate>;

TEST_CASE("[HASH_ARRAY][CONSTRUCTORS]") {

    std::vector<int> storage;

    CustomKey a { .a = 1, .b = 2 };
    CustomKey b { .a = 2, .b = 4 };
    CustomKey c { .a = 3, .b = 6 };

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

    CustomKey a { .a = 1, .b = 2 };
    CustomKey b { .a = 2, .b = 4 };
    CustomKey c { .a = 3, .b = 6 };
    CustomKey d { .a = 2, .b = 4 };

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

    CustomKey a { .a = 1, .b = 2 };
    CustomKey b { .a = 2, .b = 4 };
    CustomKey c { .a = 3, .b = 6 };

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

    CustomKey a { .a = 1, .b = 2 };
    CustomKey b { .a = 2, .b = 4 };
    CustomKey c { .a = 3, .b = 6 };

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

    CustomKey a { .a = 1, .b = 2 };
    CustomKey b { .a = 2, .b = 4 };
    CustomKey c { .a = 3, .b = 6 };

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

TEST_CASE("[TEMPLATE_HASH_ARRAY][CONSTRUCTORS]") {

    std::vector<int> storage;
    std::vector<CustomKeyTag> tags;

    constexpr CustomKeyTemplate a { .a = 1, .b = 2, .tag = CustomKeyTag::ONE };
    constexpr CustomKeyTemplate b { .a = 2, .b = 4, .tag = CustomKeyTag::TWO };
    constexpr CustomKeyTemplate c { .a = 3, .b = 6, .tag = CustomKeyTag::THREE };

    auto a_index = insert_key(a, storage, tags);
    auto b_index = insert_key(b, storage, tags);
    auto c_index = insert_key(c, storage, tags);

    KeyAdapterTemplate adapter { storage, tags };

    SUBCASE("[TEMPLATE_HASH_ARRAY][CONSTRUCTOR][DEFAULT_INIT]") {
        template_hash_array_t array;
        CHECK(array.try_insert<a.tag>(a, a_index, adapter));
        CHECK(array.try_insert<b.tag>(b, b_index, adapter));
        CHECK(array.try_insert<c.tag>(c, c_index, adapter));

        CHECK_EQ(array.size(), 3);

        array.clear();

        CHECK_EQ(array.size(), 0);
    }

    SUBCASE("[TEMPLATE_HASH_ARRAY][CONSTRUCTOR][INIT]") {
        template_hash_array_t array(4);
        CHECK_EQ(array.bucket_count(), 4);
        CHECK_EQ(array.size(), 0);
    }

    SUBCASE("[TEMPLATE_HASH_ARRAY][CONSTRUCTOR][COPY]") {
        template_hash_array_t a_array;
        CHECK(a_array.try_insert<a.tag>(a, a_index, adapter));
        CHECK(a_array.try_insert<b.tag>(b, b_index, adapter));

        REQUIRE_EQ(a_array.size(), 2);

        template_hash_array_t b_array = a_array;

        CHECK(b_array.try_insert<c.tag>(c, c_index, adapter));

        REQUIRE_EQ(b_array.size(), 3);

        b_array = a_array;

        CHECK_EQ(b_array.size(), 2);
    }

    SUBCASE("[TEMPLATE_HASH_ARRAY][CONSTRUCTOR][MOVE]") {
        template_hash_array_t a_array;
        CHECK(a_array.try_insert<a.tag>(a, a_index, adapter));
        CHECK(a_array.try_insert<b.tag>(b, b_index, adapter));

        REQUIRE_EQ(a_array.size(), 2);

        template_hash_array_t b_array = std::move(a_array);

        CHECK_EQ(b_array.size(), 2);
    }
}

TEST_CASE("[TEMPLATE_HASH_ARRAY][TRY]") {

    std::vector<int> storage;
    std::vector<CustomKeyTag> tags;

    constexpr CustomKeyTemplate a { .a = 1, .b = 2, .tag = CustomKeyTag::ONE };
    constexpr CustomKeyTemplate b { .a = 2, .b = 4, .tag = CustomKeyTag::TWO };
    constexpr CustomKeyTemplate c { .a = 3, .b = 6, .tag = CustomKeyTag::THREE };
    constexpr CustomKeyTemplate d { .a = 2, .b = 4, .tag = CustomKeyTag::TWO };

    auto a_index = insert_key(a, storage, tags);
    auto b_index = insert_key(b, storage, tags);
    auto c_index = insert_key(c, storage, tags);
    auto d_index = insert_key(d, storage, tags);

    CHECK_NE(b_index, d_index);
    CHECK_EQ(b, d);

    KeyAdapterTemplate adapter { storage, tags };

    template_hash_array_t array;
    CHECK(array.try_insert<a.tag>(a, a_index, adapter));
    CHECK(array.try_insert<b.tag>(b, b_index, adapter));
    CHECK(array.try_insert<c.tag>(c, c_index, adapter));
    CHECK(array.try_insert<c.tag>(c, c_index, adapter) == false);

    CHECK(array.try_set<c.tag>(CustomKeyTemplate { 50, 90, c.tag }, 5, adapter) == false);
    CHECK(array.try_set<c.tag>(b, 5, adapter) == false);
    CHECK(array.try_set<b.tag>(b, d_index, adapter) == true);
}

TEST_CASE("[TEMPLATE_HASH_ARRAY][FIND]") {

    std::vector<int> storage;
    std::vector<CustomKeyTag> tags;

    constexpr CustomKeyTemplate a { .a = 1, .b = 2, .tag = CustomKeyTag::ONE };
    constexpr CustomKeyTemplate b { .a = 2, .b = 4, .tag = CustomKeyTag::TWO };
    constexpr CustomKeyTemplate c { .a = 3, .b = 6, .tag = CustomKeyTag::THREE };

    auto a_index = insert_key(a, storage, tags);
    auto b_index = insert_key(b, storage, tags);
    auto c_index = insert_key(c, storage, tags);

    KeyAdapterTemplate adapter { storage, tags };

    template_hash_array_t array;
    CHECK(array.try_insert<a.tag>(a, a_index, adapter));
    CHECK(array.try_insert<b.tag>(b, b_index, adapter));
    CHECK(array.try_insert<c.tag>(c, c_index, adapter));

    auto it = array.find<b.tag>(b, adapter);

    CHECK_NE(it, array.end());
    CHECK_EQ(*it, b_index);

    it = array.find<b.tag>({ 850, 80, b.tag }, adapter);
    CHECK_EQ(it, array.end());
}

TEST_CASE("[TEMPLATE_HASH_ARRAY][ERASE]") {

    std::vector<int> storage;
    std::vector<CustomKeyTag> tags;

    constexpr CustomKeyTemplate a { .a = 1, .b = 2, .tag = CustomKeyTag::ONE };
    constexpr CustomKeyTemplate b { .a = 2, .b = 4, .tag = CustomKeyTag::TWO };
    constexpr CustomKeyTemplate c { .a = 3, .b = 6, .tag = CustomKeyTag::THREE };

    auto a_index = insert_key(a, storage, tags);
    auto b_index = insert_key(b, storage, tags);
    auto c_index = insert_key(c, storage, tags);

    KeyAdapterTemplate adapter { storage, tags };

    template_hash_array_t array;
    CHECK(array.try_insert<a.tag>(a, a_index, adapter));
    CHECK(array.try_insert<b.tag>(b, b_index, adapter));
    CHECK(array.try_insert<c.tag>(c, c_index, adapter));

    array.erase<a.tag>(a, adapter);
    array.erase<b.tag>(b, adapter);
    array.erase<c.tag>(c, adapter);

    CHECK(array.empty());
}

TEST_CASE("[TEMPLATE_HASH_ARRAY][ITERATOR]") {

    std::vector<int> storage;
    std::vector<CustomKeyTag> tags;

    constexpr CustomKeyTemplate a { .a = 1, .b = 2, .tag = CustomKeyTag::ONE };
    constexpr CustomKeyTemplate b { .a = 2, .b = 4, .tag = CustomKeyTag::TWO };
    constexpr CustomKeyTemplate c { .a = 3, .b = 6, .tag = CustomKeyTag::THREE };

    auto a_index = insert_key(a, storage, tags);
    auto b_index = insert_key(b, storage, tags);
    auto c_index = insert_key(c, storage, tags);

    KeyAdapterTemplate adapter { storage, tags };

    template_hash_array_t array;
    CHECK(array.try_insert<a.tag>(a, a_index, adapter));
    CHECK(array.try_insert<b.tag>(b, b_index, adapter));
    CHECK(array.try_insert<c.tag>(c, c_index, adapter));

    REQUIRE_EQ(array.size(), 3);

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
