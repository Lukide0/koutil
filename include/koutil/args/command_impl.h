#ifndef KOUTIL_ARGS_COMMAND_IMPL_H
#define KOUTIL_ARGS_COMMAND_IMPL_H

#include "koutil/args/command.h"
#include "koutil/args/help.h"

#include "koutil/args/result.h"

#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace koutil::args {

template <extends_result Result> bool command_t<Result>::add_command(command_t&& command) {
    if (m_cmd_map.contains(command.name())) {
        return false;
    }

    command.m_path = m_path;
    command.m_path += ' ';
    command.m_path += m_name;

    const std::uint32_t id = m_commands.size();
    m_cmd_map.emplace(command.name(), id);

    m_commands.emplace_back(std::move(command));

    return true;
}

template <extends_result Result> bool command_t<Result>::add_option(const option_t& option) {
    const auto long_name  = option.long_name();
    const auto short_name = option.short_name();

    if ((long_name.has_value() && m_long_options.contains(*long_name))
        || (short_name.has_value() && m_short_options.contains(*short_name))) {
        return false;
    }

    const std::size_t index = m_options.size();

    if (long_name.has_value()) {
        m_long_options[*long_name] = index;
    }

    if (short_name.has_value()) {
        m_short_options[*short_name] = index;
    }

    m_options.push_back(option);

    return true;
}

template <extends_result Result> void command_t<Result>::add_argument(const argument_t& argument) {
    m_arguments.push_back(argument);
}

template <extends_result Result>
void command_t<Result>::process(const char* const* args, std::uint32_t argc, result_t& result) {
    process(std::span<const char* const>(args, argc), result);
}

template <extends_result Result> void command_t<Result>::process(std::span<const char* const> args, result_t& result) {
    std::uint32_t arguments = 0;

    const std::uint32_t needed_arguments = m_arguments.size();

    auto create_argument_error = [this, needed_arguments](std::uint32_t arguments) -> std::string {
        if (m_path.empty()) {
            return std::format(
                "'{}' requires {} argument{}, but {} {} provided",
                m_name,
                needed_arguments,
                needed_arguments == 1 ? "" : "s",
                arguments,
                arguments == 1 ? "was" : "were"
            );
        }

        return std::format(
            "command '{}' requires {} argument{}, but {} {} provided",
            m_name,
            needed_arguments,
            needed_arguments == 1 ? "" : "s",
            arguments,
            arguments == 1 ? "was" : "were"
        );
    };

    for (std::uint32_t i = 0; i < args.size(); ++i) {
        if (result.need_exit()) {
            return;
        }

        std::string_view arg { args[i] };

        if (arg.starts_with('-')) {
            i = process_option(arg, args, i, result);
            continue;
        }

        auto cmd_it = m_cmd_map.find(arg);
        if (cmd_it != m_cmd_map.end()) {
            if (needed_arguments != arguments) {
                result.add_error(create_argument_error(arguments));
            }

            check_options(result);

            auto rest = args.subspan(i + 1);

            auto& cmd = m_commands[cmd_it->second];
            if (cmd.m_handle) {
                cmd.m_handle(rest, result);
            }

            cmd.process(rest, result);
            return;
        }

        if (arguments < needed_arguments) {
            m_arguments[arguments].process(arg, result);

            arguments += 1;
        } else {
            result.add_error(std::format("unknown argument: {}", arg));
        }
    }

    if (result.need_exit()) {
        return;
    }

    if (needed_arguments != arguments) {
        result.add_error(create_argument_error(arguments));
    }

    check_options(result);
}

template <extends_result Result>
std::uint32_t command_t<Result>::process_option(
    std::string_view name, std::span<const char* const> args, std::uint32_t index, result_t& result
) {
    using namespace std::string_view_literals;

    std::string_view arg = name;

    // removes first '-'
    name.remove_prefix(1);

    auto value_start = arg.find('=');
    bool has_value   = value_start != std::string_view::npos;

    std::uint32_t option_id;

    // long option
    if (name.starts_with('-')) {

        if (has_value) {
            name = name.substr(0, value_start - 2);
        }

        // removes second '-'
        std::string_view option_name = name.substr(1);

        auto option_it = m_long_options.find(option_name);
        if (option_it == m_long_options.end()) {
            result.add_error(std::format("unknown option '-{}'", name));
            return index;
        }

        option_id = option_it->second;

    } else {
        if (has_value) {
            name = name.substr(0, value_start - 1);
        }

        if (name.size() != 1) {
            result.add_error(std::format("Invalid short option '-{}': must be a single character", name));
        }

        auto option_it = m_short_options.find(name[0]);
        if (option_it == m_short_options.end()) {
            result.add_error(std::format("unknown option '{}'", name));
            return index;
        }

        option_id = option_it->second;
    }

    auto& option = m_options[option_id];

    if (!option.has_value()) {
        if (has_value) {
            result.add_error(std::format("option '-{}' requires a value", name));
            return index;
        }

        option.process(std::nullopt, result);
        return index;
    }

    std::string_view value;

    if (has_value) {
        value = arg.substr(value_start + 1);
    } else if (args.size() > index + 1) {
        index += 1;
        value = args[index];
    } else {
        result.add_error(std::format("option '-{}' requires a value", name));
        return index;
    }

    option.process(value, result);
    return index;
}

template <extends_result Result> void command_t<Result>::check_options(result_t& result) {
    for (const auto& option : m_options) {
        if (option.required() && !option.used()) {
            std::string option_display;
            if (option.long_name()) {
                option_display = std::format("--{}", *option.long_name());
                if (option.short_name()) {
                    option_display += std::format(" (-{})", *option.short_name());
                }
            } else if (option.short_name()) {
                option_display = std::format("-{}", *option.short_name());
            }

            result.add_error(std::format("required option {} was not provided", option_display));
        }
    }
}

template <extends_result Result> void command_t<Result>::show_help(std::ostream& out, std::size_t terminal_size) const {

    help_printer_t printer(*this, terminal_size);
    printer.print(out);
}

}

#endif
