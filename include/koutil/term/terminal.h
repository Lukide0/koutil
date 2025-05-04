#ifndef KOUTIL_TERM_TERMINAL_H
#define KOUTIL_TERM_TERMINAL_H

#include "koutil/term/style.h"
#include "koutil/util/utils.h"
#include <cassert>
#include <csignal>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <stack>
#include <string_view>

#if defined(OS_LINUX)
    #include <sys/ioctl.h>
    #include <sys/select.h>
    #include <termios.h>
    #include <unistd.h>
#elif defined(OS_WINDOWS)
    #define WIN32_LEAN_AND_MEAN

    #ifndef NOMINMAX
        #define NOMINMAX
    #endif

    #include <io.h>
    #include <Windows.h>
#endif

namespace koutil::term {

/**
 * @brief Enumerates the levels of color support for the terminal.
 */
enum class ColorSupport {
    COLOR16,
    COLOR256,
    TRUE_COLOR,
};

/**
 * @brief Represents the dimensions of the terminal buffer.
 */
struct dimensions {
    std::size_t width;
    std::size_t height;
};

class terminal {
public:
    enum class Error {
        NONE, /**< No error. */
        DOUBLE_INIT, /**< Double initialization error. */
        CODEPAGE_FAIL, /**< Failure to set code page error. */
        INPUT_HANDLE_ERR, /**< Error with input handle. */
        OUTPUT_HANDLE_ERR, /**< Error with output handle. */
        SETUP, /**< Terminal setup error. */
    };

    /**
     * @brief Initializes the terminal.
     * @return True if initialization succeeds, false otherwise.
     */
    static bool init();

    /**
     * @brief Registers signal handlers for the terminal.
     */
    static void register_signals();

    /**
     * @brief Rolls back changes made to the terminal.
     */
    static void rollback();

    /**
     * @brief Retrieves the level of color support for the terminal.
     * @return The color support level.
     */
    static ColorSupport color_support();

    /**
     * @brief Retrieves the current error state of the terminal.
     * @return The current error state.
     */
    static Error error();

    /**
     * @brief Checks if the terminal has encountered an error.
     * @return True if there is an error, false otherwise.
     */
    static bool has_error();

    /**
     * @brief Queries the dimensions of the terminal window.
     * @return The dimensions of the terminal window.
     */
    static dimensions query_dimensions();

    ~terminal() { exit(); }

private:
    std::stack<std::function<void()>> m_on_exit;
    Error m_error;
    bool m_has_signals           = false;
    ColorSupport m_color_support = ColorSupport::COLOR16;

    terminal() = default;

    void exit() {
        while (!m_on_exit.empty()) {
            auto& top = m_on_exit.top();
            top();

            m_on_exit.pop();
        }
    }

    static void handle_exit_signal(int /*unused*/) { rollback(); }

    static std::unique_ptr<terminal> s_instance;
};

std::unique_ptr<terminal> terminal::s_instance = nullptr;

bool terminal::init() {
    using namespace std::string_view_literals;

    if (s_instance != nullptr) {
        s_instance->m_error = Error::DOUBLE_INIT;
        return false;
    }

    s_instance.reset(new terminal());

#if defined(OS_WINDOWS)
    s_instance->m_color_support = ColorSupport::TRUE_COLOR;
#else
    std::string_view colorterm = util::safe_getenv("COLORTERM");
    if (colorterm.find("24bit") != std::string_view::npos || colorterm.find("truecolor") != std::string_view::npos) {
        s_instance->m_color_support = ColorSupport::TRUE_COLOR;
    }

    std::string_view term = util::safe_getenv("TERM");
    if (colorterm.find("256") != std::string_view::npos || term.find("256") != std::string_view::npos) {
        s_instance->m_color_support = ColorSupport::COLOR256;
    }
#endif

#if defined(OS_WINDOWS)
    HANDLE console_out = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE console_in  = GetStdHandle(STD_INPUT_HANDLE);

    DWORD mode_out = 0;
    DWORD mode_in  = 0;

    if (SetConsoleOutputCP(CP_UTF8) == false) {
        s_instance->m_error = Error::CODEPAGE_FAIL;
        return false;
    }

    if (console_in == INVALID_HANDLE_VALUE || !GetConsoleMode(console_in, &mode_in)) {
        s_instance->m_error = Error::INPUT_HANDLE_ERR;
        return false;
    }

    if (console_out == INVALID_HANDLE_VALUE || !GetConsoleMode(console_out, &mode_out)) {
        s_instance->m_error = Error::OUTPUT_HANDLE_ERR;
        return false;
    }

    s_instance->m_on_exit.push([=] { SetConsoleMode(console_in, mode_in); });
    s_instance->m_on_exit.push([=] { SetConsoleMode(console_out, mode_out); });

    mode_out |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
    mode_in |= ENABLE_VIRTUAL_TERMINAL_INPUT | ENABLE_WINDOW_INPUT;

    if (!SetConsoleMode(console_in, mode_in) || !SetConsoleMode(console_out, mode_out)) {
        s_instance->exit();
        s_instance->m_error = Error::SETUP;
        return false;
    }
#endif
    std::at_quick_exit(rollback);
    std::atexit(rollback);

    s_instance->m_on_exit.emplace([]() {
        std::cout << reset_all;
        std::cout.flush();
    });

    return true;
}

void terminal::register_signals() {
    assert(s_instance != nullptr);

    if (s_instance->m_has_signals) {
        return;
    }
    s_instance->m_has_signals = true;

    auto add_signal_handler = [](int sig) {
        auto old_handler = std::signal(sig, handle_exit_signal);
        s_instance->m_on_exit.emplace([=]() { std::signal(sig, old_handler); });
    };

    for (auto&& sig : { SIGTERM, SIGSEGV, SIGINT, SIGILL, SIGABRT, SIGFPE }) {
        add_signal_handler(sig);
    }

#ifdef OS_LINUX
    add_signal_handler(SIGQUIT);

#endif
}

void terminal::rollback() { s_instance.reset(); }

ColorSupport terminal::color_support() {
    assert(s_instance != nullptr);
    return s_instance->m_color_support;
}

terminal::Error terminal::error() {
    assert(s_instance != nullptr);
    return s_instance->m_error;
}

bool terminal::has_error() {
    assert(s_instance != nullptr);
    return s_instance->m_error != Error::NONE;
}

dimensions terminal::query_dimensions() {
#if defined(OS_LINUX)

    winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1) {
        ws.ws_row = 0;
        ws.ws_col = 0;
    }
    return { .width = ws.ws_col, .height = ws.ws_row };

#elif defined(OS_WINDOWS)
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE console_out = GetStdHandle(STD_OUTPUT_HANDLE);

    if (console_out == INVALID_HANDLE_VALUE || !GetConsoleScreenBufferInfo(console_out, &csbi)) {
        return { 0, 0 };
    }

    return { .width  = csbi.srWindow.Right - csbi.srWindow.Left + 1,
             .height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1 };

#else
    return { 0, 0 };
#endif
}
}

#endif
