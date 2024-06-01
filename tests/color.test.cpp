#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <doctest/doctest.h>
#include <koutil/term.h>

using koutil::term::Color;

struct HSV {
    std::uint16_t h;
    float s;
    float v;
};

struct color_test {
    HSV hsv;
    Color expected;
};

static constexpr auto max_err_float = 3;

static constexpr auto color_tests = std::to_array<color_test>({
    { .hsv = { .h = 138, .s = 0.26F, .v = 0.9F },  .expected = Color(169, 229, 187) },
    { .hsv = { .h = 89, .s = 0.24F, .v = 0.93F },  .expected = Color(211, 238, 182) },
    { .hsv = { .h = 55, .s = 0.3F, .v = 0.99F },   .expected = Color(252, 246, 177) },
    { .hsv = { .h = 44, .s = 0.56F, .v = 0.98F },  .expected = Color(250, 213, 110) },
    { .hsv = { .h = 40, .s = 0.83F, .v = 0.97F },  .expected = Color(247, 179, 43)  },
    { .hsv = { .h = 21, .s = 0.84F, .v = 0.97F },  .expected = Color(247, 112, 40)  },
    { .hsv = { .h = 2, .s = 0.85F, .v = 0.97F },   .expected = Color(247, 44,  37)  },
    { .hsv = { .h = 357, .s = 0.75F, .v = 0.57F }, .expected = Color(146, 37,  42)  },
    { .hsv = { .h = 349, .s = 0.65F, .v = 0.38F }, .expected = Color(96,  34,  45)  },
    { .hsv = { .h = 293, .s = 0.36F, .v = 0.18F }, .expected = Color(45,  30,  47)  },
});

bool compare_colors(Color test, Color expected, int max_err) {
    return std::abs(test.red - expected.red) <= max_err && std::abs(test.green - expected.green) <= max_err
        && std::abs(test.blue - expected.blue) <= max_err;
}

TEST_CASE("[COLOR][HSV]") {

    for (auto&& test : color_tests) {
        auto color    = Color::from_hsv(test.hsv.h, test.hsv.s, test.hsv.v);
        auto expected = test.expected;

        CHECK(compare_colors(color, expected, max_err_float));
    }
}
