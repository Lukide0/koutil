#include <iostream>
#include <koutil/koutil.h>

int main() {
    using koutil::Style;

    if (!koutil::terminal::init()) {
        std::cout << "ERROR CODE: " << static_cast<int>(koutil::terminal::error()) << std::endl;
        return 1;
    }

    constexpr auto my_style = Style::BOLD + Style::UNDERLINE;

    std::cout << "Hello " << my_style << "world" << koutil::reset_all << "!!!" << std::endl;

    return 0;
}
