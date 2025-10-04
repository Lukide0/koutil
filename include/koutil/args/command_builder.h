#ifndef KOUTIL_ARGS_COMMAND_BUILDER_H
#define KOUTIL_ARGS_COMMAND_BUILDER_H

#include "koutil/args/argument.h"
#include "koutil/args/command.h"
#include "koutil/args/handle.h"
#include "koutil/args/option.h"
#include "koutil/args/result.h"
#include <cassert>
#include <optional>
#include <string_view>
#include <utility>

namespace koutil::args {

/**
 * @brief Builder for creating command-line command.
 *
 * @details Allows adding options, arguments, and subcommands before building
 * the final @ref command_t object.
 *
 * Example:
 * @code
 * auto cmd = command_builder_t("build", "Builds the project")
 *     .add_option(option_builder_t('o', "output").description("Output file").has_value().build())
 *     .add_argument(argument_t("source", "Source file"))
 *     .add_command(command_builder_t("clean", "Cleans the project").build())
 *     .build();
 * @endcode
 */
template <extends_result Result = result_base_t> class command_builder_t {
public:
    using result_t   = Result;
    using command_t  = command_t<result_t>;
    using option_t   = option_t<result_t>;
    using argument_t = argument_t<result_t>;

    /**
     * @brief Constructs a builder for a command with name and description.
     * @param name Command name.
     * @param description Command description.
     */
    command_builder_t(std::string_view name, std::string_view description)
        : m_cmd(name, description) { }

    /**
     * @brief Constructs a builder for a command with name, description, and handler.
     * @tparam Handle Callable type compatible with `void(std::optional<std::string_view>, result_t&)`.
     * @param name Command name.
     * @param description Command description.
     * @param handle Function to execute when the command is invoked.
     */
    template <void_handle<std::optional<std::string_view>, result_t&> Handle>
    command_builder_t(std::string_view name, std::string_view description, Handle&& handle)
        : m_cmd(name, description, std::forward<Handle>(handle)) { }

    /**
     * @brief Adds an option to the command.
     * @param option Option to add.
     * @return Builder.
     */
    command_builder_t&& add_option(const option_t& option) && {
        assert(m_cmd.add_option(option));
        return std::move(*this);
    }

    /**
     * @brief Adds a positional argument to the command.
     * @param argument Argument to add.
     * @return Builder.
     */
    command_builder_t&& add_argument(const argument_t& argument) && {
        m_cmd.add_argument(argument);
        return std::move(*this);
    }

    /**
     * @brief Adds a subcommand to the command.
     * @param command Subcommand to add.
     * @return Builder.
     */
    command_builder_t&& add_command(command_t&& command) && {
        assert(m_cmd.add_command(std::move(command)));
        return std::move(*this);
    }

    /**
     * @brief Builds and returns the final command.
     * @return Constructed @ref command_t object.
     */
    command_t build() && { return std::move(m_cmd); }

private:
    command_t m_cmd;
};
}

#endif
