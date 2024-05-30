#include <iostream>
#include <koutil/term.h>

int main() {
    using koutil::term::Color;
    using namespace koutil::term::color_literals;

    if (!koutil::term::terminal::init()) {
        std::cout << "ERROR CODE: " << static_cast<int>(koutil::term::terminal::error()) << std::endl;
        return 1;
    }

    constexpr auto color_rgb     = Color::from_rgb(55, 145, 127);
    constexpr auto color_hsv     = Color::from_hsv(168, 0.62F, 0.57F);
    constexpr auto color_hex     = Color::from_hex("#37917f");
    constexpr auto color_hex_lit = "#37917f"_color;

    std::cout << color_rgb.as_fg() << "Hello" << color_hsv.as_fg() << ' ' << color_hex.as_fg() << "World"
              << color_hex_lit.as_fg() << "!!!" << koutil::term::reset_color << std::endl;

    // Alternative
    std::cout << "#F00"_fg << "Short color" << koutil::term::reset_color << std::endl;

    return 0;
}
