#ifndef KOUTIL_ARGS_HELP_H
#define KOUTIL_ARGS_HELP_H

#include "koutil/args/argument.h"
#include "koutil/args/command.h"
#include "koutil/args/option.h"
#include "koutil/args/result.h"
#include <cstddef>
#include <ostream>
#include <string_view>
#include <vector>

namespace koutil::args {

template <extends_result> class command_t;

template <extends_result Result> class help_printer_t {
public:
    using result_t      = Result;
    using command_type  = command_t<result_t>;
    using option_type   = option_t<result_t>;
    using argument_type = argument_t<result_t>;

    help_printer_t(const command_type& cmd, std::size_t terminal_size = 80)
        : m_cmd(cmd)
        , m_term_size(terminal_size) { }

    void print(std::ostream& out);

private:
    const command_type& m_cmd;
    std::size_t m_term_size;

    void print_arguments(std::ostream& out, const std::vector<argument_type>& args);
    void print_options(std::ostream& out, const std::vector<option_type>& options);
    void print_commands(std::ostream& out, const std::vector<command_type>& cmds);

    void print_item(
        std::ostream& out,
        std::size_t max_size,
        std::string_view prefix,
        std::string_view name,
        std::string_view description
    );
};

}

#endif
