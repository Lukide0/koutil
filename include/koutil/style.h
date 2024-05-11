#ifndef KOUTIL_STYLE_H
#define KOUTIL_STYLE_H

#include "koutil/util/utils.h"
#include <array>
#include <cstdint>
#include <ostream>
#include <string_view>
#include <utility>

namespace koutil {

enum class Style : std::uint8_t {
    NONE         = 0, /**< No style. */
    BOLD         = 1 << 0, /**< Bold text. */
    DIM          = 1 << 1, /**< Dim text. */
    ITALIC       = 1 << 2, /**< Italic text. */
    UNDERLINE    = 1 << 3, /**< Underlined text. */
    BLINK        = 1 << 4, /**< Blinking text. */
    INVERSE      = 1 << 5, /**< Inverted colors for text. */
    HIDDEN       = 1 << 6, /**< Hidden text. */
    STRIKETHOUGH = 1 << 7, /**< Text with a strikethrough effect. */
};

constexpr Style operator+(Style a, Style b) {
    return static_cast<Style>(std::to_underlying(a) | std::to_underlying(b));
}

constexpr Style operator-(Style a, Style b) {
    return static_cast<Style>(std::to_underlying(a) ^ (std::to_underlying(b) & std::to_underlying(a)));
}

constexpr Style operator&(Style a, Style b) {
    return static_cast<Style>(std::to_underlying(a) & std::to_underlying(b));
}

/**
 * @brief Checks if a text style contains certain flags.
 * @param style The text style to check.
 * @param flags The flags to check for.
 * @return True if the text style contains all specified flags, false otherwise.
 */
constexpr bool style_contains(Style style, Style flags) { return (style & flags) == flags; }

std::ostream& operator<<(std::ostream& stream, Style style) {
    using namespace std::string_view_literals;
    if (style == Style::NONE) {
        return stream;
    }

    /*
    00 -> "22"
    01 -> "22;1"
    10 -> "22;2"
    11 -> "1;2"
    */
    constexpr auto bold_dim_lookup = std::to_array<std::string_view>({ "22"sv, "22;1"sv, "22;2"sv, "1;2"sv });

    stream << util::ESC << '[' << bold_dim_lookup[static_cast<int>(style & (Style::BOLD + Style::DIM))];

    if (style_contains(style, Style::ITALIC)) {
        stream << ";3";
    } else {
        stream << ";23";
    }

    if (style_contains(style, Style::UNDERLINE)) {
        stream << ";4";
    } else {
        stream << ";24";
    }

    if (style_contains(style, Style::BLINK)) {
        stream << ";5";
    } else {
        stream << ";25";
    }

    if (style_contains(style, Style::INVERSE)) {
        stream << ";7";
    } else {
        stream << ";27";
    }

    if (style_contains(style, Style::HIDDEN)) {
        stream << ";8";
    } else {
        stream << ";28";
    }

    if (style_contains(style, Style::STRIKETHOUGH)) {
        stream << ";9m";
    } else {
        stream << ";29m";
    }

    return stream;
}

/**
 * @brief Resets all text styles and colors to default.
 * @param stream The output stream.
 * @return The modified output stream.
 */
std::ostream& reset_all(std::ostream& stream) {
    stream << util::ESC << "[0m";
    return stream;
}

}

#endif
