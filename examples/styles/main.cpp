#include <iostream>
#include <koutil/term.h>

int main() {
    using koutil::term::Style;

    if (!koutil::term::terminal::init()) {
        std::cout << "ERROR CODE: " << static_cast<int>(koutil::term::terminal::error()) << std::endl;
        return 1;
    }

    constexpr auto my_style = Style::BOLD + Style::UNDERLINE;

    std::cout << "Hello " << my_style << "world" << koutil::term::reset_all << "!!!" << std::endl;

    return 0;
}
