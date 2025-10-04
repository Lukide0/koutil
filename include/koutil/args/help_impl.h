#ifndef KOUTIL_ARGS_HELP_IMPL_H
#define KOUTIL_ARGS_HELP_IMPL_H

#include "koutil/args/command.h"
#include "koutil/args/help.h"
#include "koutil/args/result.h"
#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <ios>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace koutil::args {

template <extends_result Result> void help_printer_t<Result>::print(std::ostream& out) {
    // clang-format off
    out << m_cmd.description() << '\n'
        << '\n'
        << "Usage: ";
    // clang-format on

    if (!m_cmd.path().empty()) {
        out << m_cmd.path() << ' ';
    }
    out << m_cmd.name();

    const auto& options = m_cmd.options();
    const auto& args    = m_cmd.arguments();
    const auto& cmds    = m_cmd.commands();

    if (!options.empty()) {
        out << " [OPTIONS]";
    }

    if (!args.empty()) {
        out << " <ARGUMENTS>";
    }

    if (!cmds.empty()) {
        out << " [COMMANDS]";
    }

    out << '\n';

    print_arguments(out, args);
    print_options(out, options);
    print_commands(out, cmds);
}

template <extends_result Result>
void help_printer_t<Result>::print_arguments(std::ostream& out, const std::vector<argument_t>& args) {
    if (args.empty()) {
        return;
    }

    std::size_t max_size = 0;

    for (const auto& arg : args) {
        max_size = std::max(max_size, arg.name().size() + 2);
    }

    out << "Arguments:\n";

    for (const auto& arg : args) {
        print_item(out, max_size, "", arg.name(), arg.description());
    }
}

template <extends_result Result>
void help_printer_t<Result>::print_options(std::ostream& out, const std::vector<option_t>& options) {
    if (options.empty()) {
        return;
    }

    std::size_t max_size = 0;

    for (const auto& opt : options) {
        std::size_t size = 0;

        if (opt.long_name() && opt.short_name()) {
            // ", "
            size += 2;
        }

        if (opt.short_name()) {
            // "-x"
            size += 2;
        }

        if (opt.long_name()) {
            // "--name"
            size += 2 + opt.long_name()->size();
        }

        if (opt.has_value()) {
            // " <name>"
            size += 3 + opt.value_name().size();
        }

        max_size = std::max(max_size, size);
    }

    out << "Options:\n";
    for (const auto& opt : options) {
        std::string name;

        bool has_short_name = opt.short_name().has_value();

        if (has_short_name) {
            name += '-';
            name += *opt.short_name();
        }

        if (opt.long_name()) {
            if (has_short_name) {
                name += ", ";
            }

            name += "--";
            name += *opt.long_name();
        }

        if (opt.has_value()) {
            name += " <";
            name += opt.value_name();
            name += '>';
        }

        std::string_view prefix;
        if (opt.required()) {
            prefix = "(required) ";
        }

        print_item(out, max_size, prefix, name, opt.description());
    }
}

template <extends_result Result>
void help_printer_t<Result>::print_commands(std::ostream& out, const std::vector<command_t>& cmds) {
    if (cmds.empty()) {
        return;
    }

    std::size_t max_size = 0;

    for (const auto& cmd : cmds) {
        max_size = std::max(max_size, cmd.name().size());
    }

    out << "Commands:\n";

    for (const auto& cmd : cmds) {
        print_item(out, max_size, "", cmd.name(), cmd.description());
    }
}

template <extends_result Result>
void help_printer_t<Result>::print_item(
    std::ostream& out,
    std::size_t max_size,
    std::string_view prefix,
    std::string_view name,
    std::string_view description
) {
    const std::size_t indent     = 2;
    const std::size_t name_width = indent + max_size;
    const std::size_t desc_start = name_width + indent;

    out << "  " << std::left << std::setw(name_width) << name;
    const std::size_t available_width = m_term_size - desc_start;

    if (description.size() + prefix.size() <= available_width) {
        out << prefix << description << '\n';
        return;
    }

    std::string desc(prefix);
    desc += description;

    std::istringstream stream(desc);
    std::string word;

    std::size_t line_len = 0;
    bool first_line      = true;

    while (stream >> word) {
        if (!first_line && line_len + word.size() + 1 > available_width) {
            out << '\n' << std::setw(desc_start) << "";
            line_len = 0;
        }

        if (line_len > 0) {
            out << ' ';
            line_len += 1;
        }

        out << word;
        line_len += word.size();
        first_line = false;
    }

    out << '\n';
}
}

#endif
