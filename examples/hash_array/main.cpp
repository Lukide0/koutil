#include "koutil/container/template_hash_array.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <koutil/container/hash_array.h>
#include <limits>
#include <span>
#include <string>
#include <utility>
#include <vector>

struct Key {
    enum class Tag {
        NUMBER,
        CHARACTER,
    };

    Tag tag;

    union {
        char character;
        int number;
    };
};

struct Storage {
    std::vector<Key::Tag> tags;
    std::vector<std::size_t> data_index;

    std::vector<int> numbers;
    std::vector<char> chars;

    std::size_t insert(const Key& key) {
        std::size_t index = tags.size();

        tags.push_back(key.tag);

        switch (key.tag) {
        case Key::Tag::NUMBER:
            data_index.push_back(insert(key.number));
            break;
        case Key::Tag::CHARACTER:
            data_index.push_back(insert(key.character));
            break;
        }
        return index;
    }

    std::size_t insert(char data) {
        std::size_t index = chars.size();
        chars.push_back(data);
        return index;
    }

    std::size_t insert(int data) {
        std::size_t index = numbers.size();
        numbers.push_back(data);
        return index;
    }
};

class KeyAdapter {
public:
    KeyAdapter(const Storage& storage)
        : m_storage(storage) { }

    template <Key::Tag tag> [[nodiscard]] bool eql(const Key& key, std::size_t index) const {
        if (tag != m_storage.tags[index]) {
            return false;
        }

        if constexpr (tag == Key::Tag::NUMBER) {
            return m_storage.numbers[m_storage.data_index[index]] == key.number;
        } else if constexpr (tag == Key::Tag::CHARACTER) {
            return key.character == m_storage.chars[m_storage.data_index[index]];
        }
    }

private:
    const Storage& m_storage;
};

struct KeyHash {
    template <Key::Tag tag> [[nodiscard]] std::size_t hash(const Key& key) const {
        switch (tag) {
        case Key::Tag::NUMBER:
            return std::hash<int>()(key.number);
        case Key::Tag::CHARACTER:
            return std::hash<char>()(key.character);
        }
    }
};

using hash_array_t = koutil::container::template_hash_array<Key, std::size_t, Key::Tag, KeyAdapter, KeyHash>;

constexpr std::size_t N = 1000;

int main() {
    using char_limits = std::numeric_limits<char>;

    Storage storage;

    hash_array_t hash_array;

    KeyAdapter adapter { storage };

    std::cout << "Inserting " << N << " keys..." << std::endl;

    for (std::size_t i = 0; i < N; ++i) {
        const auto key_num = Key {
            .tag    = Key::Tag::NUMBER,
            .number = static_cast<int>(i % 50),
        };
        const auto key_char = Key {
            .tag    = Key::Tag::CHARACTER,
            .number = static_cast<char>(i % char_limits::max()),
        };

        if (hash_array.find<Key::Tag::NUMBER>(key_num, adapter) == hash_array.end()) {
            assert(hash_array.try_insert<Key::Tag::NUMBER>(key_num, storage.insert(key_num), adapter));
        }

        if (hash_array.find<Key::Tag::CHARACTER>(key_char, adapter) == hash_array.end()) {
            assert(hash_array.try_insert<Key::Tag::CHARACTER>(key_char, storage.insert(key_char), adapter));
        }
    }

    std::cout << "Numbers: " << storage.numbers.size() << std::endl;
    std::cout << "Chars: " << storage.chars.size() << std::endl;
}
