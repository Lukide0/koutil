#ifndef KOUTIL_TERM_COMMANDS_H
#define KOUTIL_TERM_COMMANDS_H

#include "koutil/util/utils.h"
#include <ostream>

namespace koutil::term {
/**
 * @brief Represents a cursor movement command.
 */
struct CursorPos {
    unsigned short line;
    unsigned short column;
};

/**
 * @brief Represents a cursor movement command.
 */
struct CursorMove {
    /**
     * @brief Enumerates different types of cursor movements.
     */
    enum Type : char {
        MOVE_UP              = 'A',
        MOVE_DOWN            = 'B',
        MOVE_LEFT            = 'C',
        MOVE_RIGHT           = 'D',
        MOVE_DOWN_BEGIN_LINE = 'E',
        MOVE_UP_BEGIN_LINE   = 'F',
        MOVE_COLUMN          = 'G',
    };

    unsigned short move;
    Type type;
};

/**
 * @brief Enumerates different cursor commands.
 */
enum class CursorCommand {
    MOVE_HOME, /**< Move the cursor to the home position. */
    SAVE, /**< Save the current cursor position. */
    RESTORE, /**< Restore the saved cursor position. */
    HIDE, /**< Hide the cursor. */
    SHOW, /**< Show the cursor. */
};

/**
 * @brief Enumerates different buffer commands.
 */
enum class BufferCommand : char {
    ENABLE_ALTERNATIVE_BUFFER  = 'h', /**< Enable the alternative buffer. */
    DISABLE_ALTERNATIVE_BUFFER = 'l', /**< Disable the alternative buffer. */
};

/**
 * @brief Enumerates different erase commands.
 */
enum class EraseCommand {
    ERASE_CURSOR_END, /**< Erase from the cursor to the end of the screen. */
    ERASE_CURSOR_BEGIN, /**< Erase from the cursor to the beginning of the screen. */
    ERASE_SCREEN, /**< Erase the entire screen. */
    ERASE_LINE, /**< Erase the current line. */
    ERASE_LINE_CURSOR_END, /**< Erase from the cursor to the end of the line. */
    ERASE_LINE_CURSOR_BEGIN, /**< Erase from the cursor to the beginning of the line. */
};

std::ostream& operator<<(std::ostream& stream, CursorPos pos) {
    stream << util::ESC << '[' << pos.line << ';' << pos.column << 'H';
    return stream;
}

std::ostream& operator<<(std::ostream& stream, CursorMove move) {
    stream << util::ESC << '[' << move.move << static_cast<char>(move.type);
    return stream;
}

std::ostream& operator<<(std::ostream& stream, CursorCommand cmd) {
    stream << util::ESC;

    switch (cmd) {
    case CursorCommand::MOVE_HOME:
        stream << "[H";
        break;
    case CursorCommand::SAVE:
        stream << " 7";
        break;
    case CursorCommand::RESTORE:
        stream << " 8";
        break;
    case CursorCommand::HIDE:
        stream << "[?25l";
        break;
    case CursorCommand::SHOW:
        stream << "[?25h";
        break;
    }

    return stream;
}

std::ostream& operator<<(std::ostream& stream, BufferCommand cmd) {
    stream << util::ESC << "[?1049" << static_cast<char>(cmd);
    return stream;
}

std::ostream& operator<<(std::ostream& stream, EraseCommand cmd) {
    stream << util::ESC << '[';

    switch (cmd) {
    case EraseCommand::ERASE_CURSOR_END:
        stream << "0J";
        break;
    case EraseCommand::ERASE_CURSOR_BEGIN:
        stream << "1J";
        break;
    case EraseCommand::ERASE_SCREEN:
        stream << "2J";
        break;
    case EraseCommand::ERASE_LINE_CURSOR_END:
        stream << "0K";
        break;
    case EraseCommand::ERASE_LINE_CURSOR_BEGIN:
        stream << "1K";
        break;
    case EraseCommand::ERASE_LINE:
        stream << "2K";
        break;
    }

    return stream;
}

}

#endif
