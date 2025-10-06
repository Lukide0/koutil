#ifndef KOUTIL_ARGS_PARSER_H
#define KOUTIL_ARGS_PARSER_H

#include "koutil/args/argument.h"
#include "koutil/args/command.h"
#include "koutil/args/option.h"
#include "koutil/args/result.h"
#include <cstdint>
#include <iostream>
#include <span>
#include <string_view>
#include <utility>

#include "koutil/args/command_impl.h"
#include "koutil/args/help_impl.h"

namespace koutil::args {

/**
 * @brief Command-line parser.
 * @tparam Result Type of the result object. Must extend @ref result_base_t.
 *                Defaults to @ref result_base_t.
 */
template <extends_result Result = result_base_t> class parser_t {
public:
    using result_t   = Result;
    using command_t  = command_t<result_t>;
    using option_t   = option_t<result_t>;
    using argument_t = argument_t<result_t>;

    /**
     * @brief Constructs a parser with a name and description.
     * @param name Parser name.
     * @param description Parser description.
     */
    parser_t(std::string_view name, std::string_view description);

    /**
     * @brief Constructs a parser from command.
     * @param main Command.
     */
    parser_t(command_t cmd);

    /**
     * @brief Gets the parser name
     * @return Parser name
     */
    [[nodiscard]] std::string_view name() const { return m_main_command.name(); }

    /**
     * @brief Gets the parser description
     * @return Parser description
     */
    [[nodiscard]] std::string_view description() const { return m_main_command.description(); }

    /**
     * @brief Adds a subcommand.
     * @param command Command to add.
     * @return True if added successfully.
     */
    bool add_command(command_t&& command);

    /**
     * @brief Adds an option to the parser.
     * @param option Option to add.
     * @return True if added successfully.
     */
    bool add_option(const option_t& option);

    /**
     * @brief Adds a positional argument.
     * @param argument Argument to add.
     */
    void add_argument(const argument_t& argument);

    /**
     * @brief Parses command-line arguments.
     * @param args Argument array.
     * @param argc Number of arguments.
     * @return Parsing result.
     */
    result_t parse(const char* const* args, std::uint32_t argc);

    /**
     * @brief Parses command-line arguments.
     * @param args Argument span.
     * @return Parsing result.
     */
    result_t parse(std::span<const char* const> args);

    /**
     * @brief Displays help text.
     * @param out Output stream.
     * @param terminal_size Width of the terminal.
     */
    void show_help(std::ostream& out = std::cout, std::size_t terminal_size = 80) const {
        m_main_command.show_help(out, terminal_size);
    }

    /**
     * @brief Clears state
     */
    void clear_used() { m_main_command.clear_used(); }

private:
    std::string_view m_version;
    command_t m_main_command;
};

template <extends_result Result>
parser_t<Result>::parser_t(command_t cmd)
    : m_main_command(std::move(cmd)) { }

template <extends_result Result>
parser_t<Result>::parser_t(std::string_view name, std::string_view description)
    : m_main_command(name, description) { }

template <extends_result Result> bool parser_t<Result>::add_command(command_t&& command) {
    return m_main_command.add_command(std::move(command));
}

template <extends_result Result> bool parser_t<Result>::add_option(const option_t& option) {
    return m_main_command.add_option(option);
}

template <extends_result Result> void parser_t<Result>::add_argument(const argument_t& argument) {
    m_main_command.add_argument(argument);
}

template <extends_result Result> Result parser_t<Result>::parse(const char* const* args, std::uint32_t argc) {
    return parse(std::span<const char* const>(args, argc));
}

template <extends_result Result> Result parser_t<Result>::parse(std::span<const char* const> args) {
    result_t result;
    m_main_command.process(args, result);

    return result;
}

}

#endif
