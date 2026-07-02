#ifndef KOUTIL_ARGS_COMMAND_H
#define KOUTIL_ARGS_COMMAND_H

#include "koutil/args/argument.h"
#include "koutil/args/handle.h"
#include "koutil/args/option.h"
#include "koutil/args/result.h"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace koutil::args {

/**
 * @brief Represents a command in the command-line interface.
 *
 * @details A command can contain subcommands, options, and arguments.
 *
 * The order in which options, arguments, and subcommands are added determines
 * their order in the help text. The order of arguments also defines the order
 * in which they are expected on the command line.
 */
template <extends_result Result = result_base_t> class command_t {
public:
    using result_t      = Result;
    using option_type   = option_t<result_t>;
    using argument_type = argument_t<result_t>;
    /**
     * @brief Function handle type for command execution.
     *
     * The handle is called when the parser encounters this command,
     * and it runs **before** processing the command’s arguments, options, or subcommands.
     */
    using handle_t = std::function<void(std::span<const char* const>, result_t&)>;

    /**
     * @brief Constructs a command with a name, description, and handler.
     * @tparam Handle Callable matching void_handle<std::span<const char* const>, result_t&>.
     * @param name Command name.
     * @param description Description shown in help.
     * @param handle Function to execute when the command is invoked.
     */
    template <void_handle<std::span<const char* const>, result_t&> Handle>
    explicit command_t(std::string_view name, std::string_view description, Handle&& handle)
        : m_name(name)
        , m_description(description)
        , m_handle(std::forward<Handle>(handle)) { }

    /**
     * @brief Constructs a command with a name and description, without a handler.
     * @param name Command name.
     * @param description Command description.
     */
    explicit command_t(std::string_view name, std::string_view description)
        : m_name(name)
        , m_description(description)
        , m_handle(nullptr) { }

    /**
     * @brief Gets the command name.
     * @return Command name.
     */
    [[nodiscard]] std::string_view name() const { return m_name; }

    /**
     * @brief Gets the command description.
     * @return Command description.
     */
    [[nodiscard]] std::string_view description() const { return m_description; }

    /**
     * @brief Gets the list of positional arguments.
     * @return Arguments.
     */
    [[nodiscard]] const auto& arguments() const { return m_arguments; }

    /**
     * @brief Gets the list of options.
     * @return Options.
     */
    [[nodiscard]] const auto& options() const { return m_options; }

    /**
     * @brief Gets the list of subcommands.
     * @return Commands.
     */
    [[nodiscard]] const auto& commands() const { return m_commands; }

    /**
     * @brief Returns the full path of parent commands, excluding this command’s name.
     *
     * @details For example:
     * - If this command is `"library"` under `"program build"`, `path()` returns `"program build"`.
     * - For a top-level command, it returns an empty string.
     */
    const std::string& path() const { return m_path; }

    /**
     * @brief Adds a subcommand.
     * @param command Command to add.
     * @return True if added successfully.
     * @note The order of added commands affects their order in the help text.
     */
    bool add_command(command_t&& command);

    /**
     * @brief Adds an option to the command.
     * @param option Option to add.
     * @return True if added successfully.
     * @note The order of added options affects their order in the help text.
     */
    bool add_option(const option_type& option);

    /**
     * @brief Adds a positional argument.
     * @param argument Argument to add.
     * @note The order of added arguments defines their expected order on the command line
     *       and their order in the help text.
     */
    void add_argument(const argument_type& argument);

    /**
     * @brief Processes command-line arguments.
     * @param args Argument array.
     * @param argc Number of arguments.
     * @param result Result object to update.
     */
    void process(const char* const* args, std::uint32_t argc, result_t& result);

    /**
     * @brief Processes command-line arguments.
     * @param args Argument span.
     * @param result Result object to update.
     */
    void process(std::span<const char* const> args, result_t& result);

    /**
     * @brief Displays help text for this command.
     * @param out Output stream.
     * @param terminal_size Terminal width for formatting.
     */
    void show_help(std::ostream& out, std::size_t terminal_size = 80) const;

    /**
     * @brief Clears state
     */
    void clear_used();

private:
    std::uint32_t
    process_option(std::string_view name, std::span<const char* const> args, std::uint32_t index, result_t& result);

    void check_options(result_t& result);

    struct string_hash {
        using is_transparent = void;

        [[nodiscard]] std::size_t operator()(std::string_view s) const { return std::hash<std::string_view>()(s); }

        [[nodiscard]] std::size_t operator()(const std::string& s) const { return std::hash<std::string>()(s); }
    };

    std::string m_path;

    std::string_view m_name;
    std::string_view m_description;
    handle_t m_handle;

    std::vector<argument_type> m_arguments;
    std::vector<option_type> m_options;
    std::vector<command_t> m_commands;
    std::unordered_map<std::string_view, std::uint32_t, string_hash> m_cmd_map;

    std::unordered_map<std::string_view, std::uint32_t, string_hash> m_long_options;
    std::unordered_map<char, std::uint32_t> m_short_options;
};

}

#endif
