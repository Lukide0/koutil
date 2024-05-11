#include <iostream>
#include <koutil/koutil.h>

int main() {
    using koutil::BufferCommand;
    using koutil::CursorCommand;

    if (!koutil::terminal::init()) {
        std::cout << "ERROR CODE: " << static_cast<int>(koutil::terminal::error()) << std::endl;
        return 1;
    }

    std::cout << BufferCommand::ENABLE_ALTERNATIVE_BUFFER << CursorCommand::MOVE_HOME << std::flush;

    std::cout << "Alternative buffer!\n";

    std::cin.get();

    std::cout << BufferCommand::DISABLE_ALTERNATIVE_BUFFER << std::flush;

    std::cout << "Normal buffer!\n";

    return 0;
}
