#include <iostream>
#include <koutil/term.h>

int main() {
    using koutil::term::BufferCommand;
    using koutil::term::CursorCommand;

    if (!koutil::term::terminal::init()) {
        std::cout << "ERROR CODE: " << static_cast<int>(koutil::term::terminal::error()) << std::endl;
        return 1;
    }

    std::cout << BufferCommand::ENABLE_ALTERNATIVE_BUFFER << CursorCommand::MOVE_HOME << std::flush;

    std::cout << "Alternative buffer!\n";

    std::cin.get();

    std::cout << BufferCommand::DISABLE_ALTERNATIVE_BUFFER << std::flush;

    std::cout << "Normal buffer!\n";

    return 0;
}
