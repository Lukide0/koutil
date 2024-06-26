#ifndef KOUTIL_TERM_COLOR_H
#define KOUTIL_TERM_COLOR_H

#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace koutil::term {

struct ColorBG;
struct ColorFG;

/**
 * @brief Structure representing a color.
 *
 * The Color structure represents a color with red, green, and blue channels.
 * It can also represent a color using an ID value. Color objects can be
 * created from RGB values, ID values, hexadecimal strings, or HSV values.
 */
struct Color {
public:
    using channel_t = std::uint8_t;

    enum class Tag : std::uint8_t {
        RGB,
        ID,
    };

    const channel_t red;
    const channel_t green;
    const channel_t blue;
    const Tag tag;

public:
    constexpr Color()
        : red()
        , green()
        , blue()
        , tag(Tag::RGB) { }

    /**
     * @brief Constructor with RGB values.
     *
     * Initializes the color with the specified RGB values and the RGB tag.
     *
     * @param r Red channel value.
     * @param g Green channel value.
     * @param b Blue channel value.
     */
    constexpr Color(channel_t r, channel_t g, channel_t b)
        : red(r)
        , green(g)
        , blue(b)
        , tag(Tag::RGB) { }

    /**
     * @brief Constructor with an ID value.
     *
     * Initializes the color with the specified ID value and the ID tag.
     *
     * @param id ID value.
     */
    constexpr Color(std::uint8_t id)
        : red(id)
        , green()
        , blue()
        , tag(Tag::ID) { }

    /**
     * @brief Creates a Color object from RGB values.
     *
     * @param red Red channel value.
     * @param green Green channel value.
     * @param blue Blue channel value.
     * @return Color object.
     */
    static constexpr Color from_rgb(channel_t red, channel_t green, channel_t blue) { return { red, green, blue }; }

    /**
     * @brief Creates a Color object from an ID value.
     *
     * @param id ID value.
     * @return Color object.
     */
    static constexpr Color from_id(channel_t id) { return { id }; }

    /**
     * @brief Creates a Color object from HSV values.
     *
     * @param h Hue value (0-359).
     * @param s Saturation value (0.0-1.0).
     * @param v Value (0.0-1.0).
     * @return Color object.
     */
    static constexpr Color from_hsv(std::uint16_t h, float s, float v);

    /**
     * @brief Creates a Color object from a hexadecimal string.
     *
     * @param hex Hexadecimal string (e.g., "#RRGGBB" or "#RGB").
     * @return Color object.
     */
    static consteval Color from_hex(std::string_view hex) {
        if (!hex.starts_with('#')) {
            assert(false);
        }

        if (hex.size() - 1 == 3) {
            return {
                static_cast<channel_t>(extract_value(hex[1]) | (extract_value(hex[1]) << 4)),
                static_cast<channel_t>(extract_value(hex[2]) | (extract_value(hex[2]) << 4)),
                static_cast<channel_t>(extract_value(hex[3]) | (extract_value(hex[3]) << 4)),
            };
        } else if (hex.size() - 1 == 6) {
            return {
                static_cast<channel_t>(extract_value(hex[2]) | (extract_value(hex[1]) << 4)),
                static_cast<channel_t>(extract_value(hex[4]) | (extract_value(hex[3]) << 4)),
                static_cast<channel_t>(extract_value(hex[6]) | (extract_value(hex[5]) << 4)),
            };
        } else {
            assert(false);
        }
    }

    /**
     * @brief Gets the RGB channel values of the color.
     *
     * @return Array containing red, green, and blue channel values.
     */
    [[nodiscard]] constexpr std::array<channel_t, 3> get_channels() const {
        assert(tag == Tag::RGB);
        return { red, green, blue };
    }

    /**
     * @brief Gets the ID value of the color.
     *
     * @return ID value.
     */
    [[nodiscard]] constexpr channel_t id() const { return red; }

    /**
     * @brief Converts the color to a ColorBG object.
     *
     * @return ColorBG object.
     */
    [[nodiscard]] constexpr ColorBG as_bg() const;

    /**
     * @brief Converts the color to a ColorFG object.
     *
     * @return ColorFG object.
     */
    [[nodiscard]] constexpr ColorFG as_fg() const;

    constexpr bool operator==(Color other) const {
        return red == other.red && green == other.green && blue == other.blue && tag == other.tag;
    }

private:
    /**
     * @brief Extracts a value from a hexadecimal character.
     *
     * @param val Hexadecimal character.
     * @return Extracted value.
     */
    static consteval std::uint8_t extract_value(char val) {
        if ('a' <= val && val <= 'f') {
            return static_cast<std::uint8_t>(10 + val - 'a');
        } else if ('A' <= val && val <= 'F') {
            return static_cast<std::uint8_t>(10 + val - 'A');
        } else {
            return static_cast<std::uint8_t>(val - '0');
        }
    }

    /**
     * @brief Converts a floating-point value to a channel value.
     *
     * @param val Floating-point value.
     * @return Channel value.
     */
    static constexpr std::uint8_t convert_val(float val) {
        if (std::is_constant_evaluated()) {
            return static_cast<std::uint8_t>(val * 255.0F + 0.5F);
        } else {
            return static_cast<std::uint8_t>(std::round(val * 255.0F));
        }
    }
};

/**
 * @brief Structure representing a background color.
 */
struct ColorBG {
    constexpr ColorBG(Color c)
        : color(c) { }

    const Color color;
};

/**
 * @brief Structure representing a foreground color.
 */
struct ColorFG {
    constexpr ColorFG(Color c)
        : color(c) { }

    const Color color;
};

constexpr ColorBG Color::as_bg() const { return { *this }; }

constexpr ColorFG Color::as_fg() const { return { *this }; }

constexpr Color Color::from_hsv(std::uint16_t h, float s, float v) {
    constexpr std::uint16_t HUE_PART = 360 / 6;
    constexpr float HUE_PART_FLOAT   = 360.0F / 6.0F;

    if (s <= 0.0001F) {
        channel_t val = convert_val(v);
        return { val, val, val };
    }

    h %= 360;

    const auto region = h / HUE_PART;
    const auto rem    = static_cast<float>(h % HUE_PART) / HUE_PART_FLOAT;

    const float p = v * (1.0F - s);
    const float q = v * (1.0F - (s * rem));
    const auto t  = v * (1.0F - (s * (1.0F - rem)));

    float r = 0.F;
    float g = 0.F;
    float b = 0.F;

    switch (region) {
    case 0:
        r = v;
        g = t;
        b = p;
        break;
    case 1:
        r = q;
        g = v;
        b = p;
        break;
    case 2:
        r = p;
        g = v;
        b = t;
        break;
    case 3:
        r = p;
        g = q;
        b = v;
        break;
    case 4:
        r = t;
        g = p;
        b = v;
        break;
    case 5:
    default:
        r = v;
        g = p;
        b = q;
        break;
    }

    return { convert_val(r), convert_val(g), convert_val(b) };
}

}

#endif
