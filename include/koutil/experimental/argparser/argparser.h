#ifndef KOUTIL_ARGPARSER_ARGPARSER_H
#define KOUTIL_ARGPARSER_ARGPARSER_H

#include "koutil/experimental/argparser/arg.h"
#include "koutil/experimental/argparser/subcommand.h"
#include <cassert>
#include <concepts>
#include <cstddef>
#include <ostream>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace koutil::experimental::argparser {

enum class ParseResult {
    OK,
    ERR,
    ERR_EMPTY_OPTION,
    ERR_UNKNOWN,
    ERR_INVALID_VALUE,
    ERR_MISSING_VALUE,
};

template <typename T, typename Ctx>
concept parser = requires(Ctx& ctx, const Arg& arg, std::string_view value) {
    { T::parse_argument(ctx, value) } -> std::same_as<ParseResult>;
    { T::parse_option_flag(ctx, arg) } -> std::same_as<ParseResult>;
    { T::parse_option_value(ctx, arg, value) } -> std::same_as<ParseResult>;
    { T::parse_command(ctx, value) } -> std::same_as<ParseResult>;
};

template <typename Ctx, parser<Ctx> UParser, arguments Args, subcommand... Commands> class Parser {
public:
    constexpr Parser(Ctx& ctx, Args&& args, Commands&&... commands)
        : m_args(std::move(args))
        , m_subcommands(commands...)
        , m_ctx(ctx) {
        assert(check_subcommands(std::forward<decltype(commands)>(commands)...));
    }

    constexpr Parser(Ctx& ctx, const Args& args, const Commands&... commands)
        : m_args(args)
        , m_subcommands(commands...)
        , m_ctx(ctx) {
        assert(check_subcommands(std::forward<decltype(commands)>(commands)...));
    }

    ParseResult parse(std::size_t argc, const char** argv);
    void print_help(std::ostream& stream);

private:
    Args m_args;
    ArgumentsBase const* m_current_args = &m_args;
    std::tuple<Commands...> m_subcommands;
    Ctx& m_ctx;
    std::span<const char*> m_values;
    std::size_t m_offset     = 0;
    bool m_inside_subcommand = false;

    ParseResult parse_long_opt(std::string_view opt);
    ParseResult parse_short_opt(std::string_view opt);
    ParseResult parse_command_or_argument(std::string_view opt);

    template <subcommand Cmd, subcommand... Other>
        requires(sizeof...(Other) >= 1)
    bool check_subcommands(Cmd&& cmd, Other&&... other) {
        return ((cmd.name != other.name) && ...) && check_subcommands(other...);
    }

    template <subcommand... Other>
        requires(sizeof...(Other) <= 1)
    bool check_subcommands(Other&&... /*unused*/) {
        return true;
    }

    template <std::size_t I> ParseResult parse_command_or_argument_impl(std::string_view opt) {
        if (m_inside_subcommand) {
            return UParser::parse_argument(m_ctx, opt);
        }

        auto& cmd = std::get<I>(m_subcommands);

        if (cmd.name == opt) {
            ParseResult result = UParser::parse_command(m_ctx, opt);
            if (result != ParseResult::OK) {
                return result;
            }

            m_current_args      = static_cast<ArgumentsBase const*>(&cmd.args);
            m_inside_subcommand = true;
            return result;

        } else if constexpr (I + 1 < std::tuple_size_v<decltype(m_subcommands)>) {
            return parse_command_or_argument_impl<I + 1>(opt);
        } else {
            return UParser::parse_argument(m_ctx, opt);
        }
    }
};

template <typename Ctx, parser<Ctx> UParser, arguments Args, subcommand... Commands>
constexpr auto make_parser(Args&& args, Ctx& ctx, Commands&&... commands) {
    return Parser<Ctx, UParser, Args, Commands...>(
        ctx, std::forward<decltype(args)>(args), std::forward<decltype(commands)>(commands)...
    );
}

template <typename Ctx, parser<Ctx> UParser, arguments Args, subcommand... Commands>
ParseResult process_args(std::size_t argc, const char** argv, Args&& args, Ctx& ctx, Commands&&... commands) {
    return Parser<Ctx, UParser, Args, Commands...>(
               ctx, std::forward<decltype(args)>(args), std::forward<decltype(commands)>(commands)...
    )
        .parse(argc, argv);
}

template <typename Ctx, parser<Ctx> UParser, arguments Args, subcommand... Commands>
ParseResult Parser<Ctx, UParser, Args, Commands...>::parse(std::size_t argc, const char** argv) {
    m_values = std::span(argv + 1, argc - 1);

    for (; m_offset < m_values.size(); ++m_offset) {
        std::string_view arg(m_values[m_offset]);

        ParseResult result;
        if (arg.starts_with("--")) {
            result = parse_long_opt(arg.substr(2));
        } else if (arg.starts_with('-')) {
            result = parse_short_opt(arg.substr(1));
        } else {
            result = parse_command_or_argument(arg);
        }

        if (result != ParseResult::OK) {
            return result;
        }
    }

    return ParseResult::OK;
}

template <typename Ctx, parser<Ctx> UParser, arguments Args, subcommand... Commands>
ParseResult Parser<Ctx, UParser, Args, Commands...>::parse_short_opt(std::string_view opt) {

    if (opt.empty()) {
        return ParseResult::ERR_EMPTY_OPTION;
    }

    while (!opt.empty()) {
        auto arg_res = m_current_args->find_short(opt.front());
        if (!arg_res.has_value()) {
            return ParseResult::ERR_UNKNOWN;
        }

        const auto& arg = *arg_res.value();
        auto result     = UParser::parse_option_flag(m_ctx, arg);
        if (result != ParseResult::OK) {
            return result;
        }

        opt = opt.substr(1);
    }

    return ParseResult::OK;
}

template <typename Ctx, parser<Ctx> UParser, arguments Args, subcommand... Commands>
ParseResult Parser<Ctx, UParser, Args, Commands...>::parse_long_opt(std::string_view opt) {

    if (opt.empty()) {
        m_offset += 1;
        for (; m_offset < m_values.size(); ++m_offset) {
            auto result = UParser::parse_argument(m_ctx, m_values[m_offset]);
            if (result != ParseResult::OK) {
                return result;
            }
        }
        return ParseResult::OK;
    }

    auto eql_it             = opt.find('=', 1);
    std::string_view option = opt.substr(0, eql_it);
    std::string_view value;

    auto arg_res = m_current_args->find_long(option);
    if (!arg_res.has_value()) {
        return ParseResult::ERR_UNKNOWN;
    }

    const auto& arg = *arg_res.value();

    ParseResult result;
    switch (arg.type) {
    case Arg::OPTION_FLAG:
        result = UParser::parse_option_flag(m_ctx, arg);
        break;
    case Arg::OPTION_VALUE:
        if (eql_it != std::string_view::npos) {
            value = opt.substr(eql_it + 1);
        } else if (m_offset + 1 >= m_values.size()) {
            return ParseResult::ERR_MISSING_VALUE;
        } else {
            value = m_values[++m_offset];
        }

        result = UParser::parse_option_value(m_ctx, arg, value);
        break;
    default:
        std::unreachable();
    }

    if (result != ParseResult::OK) {
        return result;
    }
    return ParseResult::OK;
}

template <typename Ctx, parser<Ctx> UParser, arguments Args, subcommand... Commands>
ParseResult Parser<Ctx, UParser, Args, Commands...>::parse_command_or_argument(std::string_view opt) {
    if (opt.empty()) {
        return ParseResult::OK;
    } else if constexpr (std::tuple_size_v<decltype(m_subcommands)> == 0) {
        return UParser::parse_argument(m_ctx, opt);
    } else {
        return parse_command_or_argument_impl<0>(opt);
    }
}

template <typename Ctx, parser<Ctx> UParser, arguments Args, subcommand... Commands>
void Parser<Ctx, UParser, Args, Commands...>::print_help(std::ostream& stream) {

    stream << "usage: <program>";

    std::vector<std::pair<std::string, std::string_view>> options;
    std::size_t max_width = 0;

    for (const Arg& arg : m_args.args()) {
        std::string arg_help;

        switch (arg.type) {
        case Arg::OPTION_FLAG:
            if (arg.short_name != '\0') {
                arg_help = '-';
                arg_help += arg.short_name;
                arg_help += ' ';
            }

            if (!arg.long_name.empty()) {
                arg_help = "--";
                arg_help += arg.long_name;
                arg_help += ' ';
            }

            break;

        case Arg::OPTION_VALUE:
            arg_help += "--";
            arg_help += arg.long_name;
            arg_help += "=value";
            break;
        }
        max_width = std::max(max_width, arg_help.size());
        options.emplace_back(std::move(arg_help), arg.description);
    }
    if (!options.empty()) {
        stream << " [options]\n\n";
        stream << "Options:\n";

        for (auto&& [arg, desc] : options) {
            stream << "  " << arg << std::string(max_width - arg.size(), ' ') << ' ' << desc << '\n';
        }
    }
}

}

#endif
