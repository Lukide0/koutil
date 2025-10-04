#ifndef KOUTIL_ARGS_RESULT_H
#define KOUTIL_ARGS_RESULT_H

#include <concepts>
#include <string>
#include <utility>
#include <vector>

namespace koutil::args {

/**
 * @brief Represents the result of command-line parsing.
 *
 * @details The order in which options, arguments, and commands are added is significant:
 * - The order of **arguments** defines the expected order on the command line.
 * - The order of **options**, **arguments**, and **commands** determines how they appear in the help text.
 */
class result_base_t {
public:
    result_base_t() = default;

    /**
     * @brief Checks if any errors occurred.
     * @return True if there are errors.
     */
    [[nodiscard]] bool has_error() const { return !m_errors.empty(); }

    /**
     * @brief Checks if the parser requested program exit.
     * @return True if exit is required.
     * @note Can be used for options like <tt>--help</tt> to skip further
     * validation and avoid errors when required arguments are missing.
     */
    [[nodiscard]] bool need_exit() const { return m_exit; }

    /**
     * @brief Returns the list of error messages.
     * @return Errors.
     */
    [[nodiscard]] const auto& errors() const { return m_errors; }

    /**
     * @brief Adds an error message.
     * @param error Error text.
     */
    void add_error(std::string error) { m_errors.emplace_back(std::move(error)); }

    /**
     * @brief Marks the result as requiring exit.
     * @note Useful for commands like <tt>--help</tt> to stop parsing.
     */
    void exit() { m_exit = true; }

private:
    bool m_exit = false;
    std::vector<std::string> m_errors;
};

template <typename T>
concept extends_result = std::derived_from<T, result_base_t>;

}

#endif
