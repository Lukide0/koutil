#ifndef KOUTIL_TERM_COLOR_OPERATORS_H
#define KOUTIL_TERM_COLOR_OPERATORS_H

#include "koutil/term/color.h"
#include "koutil/util/utils.h"
#include <cstddef>
#include <ostream>
#include <string_view>

namespace koutil::term {

namespace color_literals {

    /**
     * @brief Creates a Color object from a hexadecimal string.
     *
     * @param str The hexadecimal string.
     * @param size The size of the string.
     * @return Color The Color object created from the hexadecimal string.
     */
    consteval color_t operator""_color(const char* str, std::size_t size) {
        std::string_view hex(str, size);
        return color_t::from_hex(hex);
    }

    /**
     * @brief Creates a ColorBG object from a hexadecimal string.
     *
     * @param str The hexadecimal string.
     * @param size The size of the string.
     * @return ColorBG The ColorBG object created from the hexadecimal string.
     */
    consteval color_bg_t operator""_bg(const char* str, std::size_t size) {
        std::string_view hex(str, size);
        return color_t::from_hex(hex).as_bg();
    }

    /**
     * @brief Creates a ColorFG object from a hexadecimal string.
     *
     * @param str The hexadecimal string.
     * @param size The size of the string.
     * @return ColorFG The ColorFG object created from the hexadecimal string.
     */
    consteval color_fg_t operator""_fg(const char* str, std::size_t size) {
        std::string_view hex(str, size);
        return color_t::from_hex(hex).as_fg();
    }

}

std::ostream& operator<<(std::ostream& stream, color_bg_t bg) {

    switch (bg.color.tag) {
    case color_t::Tag::RGB:
        stream << util::ESC << "[48;2;" << +bg.color.red << ';' << +bg.color.green << ';' << +bg.color.blue << 'm';
        break;
    case color_t::Tag::ID:
        stream << util::ESC << '[' << +bg.color.id() + 10 << 'm';
        break;
    }

    return stream;
}

std::ostream& operator<<(std::ostream& stream, color_fg_t fg) {

    switch (fg.color.tag) {
    case color_t::Tag::RGB:
        stream << util::ESC << "[38;2;" << +fg.color.red << ';' << +fg.color.green << ';' << +fg.color.blue << 'm';
        break;
    case color_t::Tag::ID:
        stream << util::ESC << '[' << +fg.color.id() << 'm';
        break;
    }

    return stream;
}

/**
 * @brief Resets background color to default.
 * @param stream The output stream.
 * @return The modified output stream.
 */
std::ostream& reset_bg(std::ostream& stream) {
    stream << util::ESC << "[49m";
    return stream;
}

/**
 * @brief Resets foreground color to default.
 * @param stream The output stream.
 * @return The modified output stream.
 */
std::ostream& reset_fg(std::ostream& stream) {
    stream << util::ESC << "[39m";
    return stream;
}

/**
 * @brief Resets foreground and background color to default.
 * @param stream The output stream.
 * @return The modified output stream.
 */
std::ostream& reset_color(std::ostream& stream) {
    stream << util::ESC << "[39;49m";
    return stream;
}

}

#endif
