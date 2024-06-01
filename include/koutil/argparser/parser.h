#ifndef KOUTIL_ARGPARSER_PARSER_H
#define KOUTIL_ARGPARSER_PARSER_H

#include "koutil/argparser/arg.h"
#include "koutil/argparser/subcommand.h"
#include <cassert>
#include <concepts>
#include <cstddef>
#include <span>
#include <string_view>
#include <utility>

namespace koutil::argparser {

/**
 * @brief Enum class representing possible parse results.
 */
enum class ParseResult {
    OK, /**< Parsing succeeded. */
    ERR, /**< General error during parsing. */
    ERR_EMPTY_OPTION, /**< Error due to empty option. */
    ERR_UNKNOWN, /**< Error due to unknown option or command. */
    ERR_INVALID_VALUE, /**< Error due to invalid value for an option. */
    ERR_MISSING_VALUE, /**< Error due to missing value for an option. */
};

/**
 * @brief Concept to check if a type is a parser.
 *
 * @tparam T The parser type.
 * @tparam Ctx The context type.
 */
template <typename T, typename Ctx>
concept parser = requires(Ctx& ctx, const Arg& arg, std::string_view value, const SubcommandBase& cmd) {
    { T::parse_argument(ctx, value) } -> std::same_as<ParseResult>;
    { T::parse_option_flag(ctx, arg) } -> std::same_as<ParseResult>;
    { T::parse_option_value(ctx, arg, value) } -> std::same_as<ParseResult>;
    { T::parse_command(ctx, cmd) } -> std::same_as<ParseResult>;
};

/**
 * @brief Template class representing a parser.
 *
 * @tparam Ctx The context type.
 * @tparam UParser The user-defined parser type.
 * @tparam Args The argument types.
 * @tparam Cmds The command types.
 */
template <typename Ctx, parser<Ctx> UParser, are_arguments Args, are_commands Cmds> class Parser {
public:
    constexpr Parser(Ctx& ctx, Args&& args, Cmds&& commands)
        : m_args(std::move(args))
        , m_subcommands(std::move(commands))
        , m_ctx(ctx) { }

    constexpr Parser(Ctx& ctx, const Args& args, const Cmds& commands)
        : m_args(args)
        , m_subcommands(commands)
        , m_ctx(ctx) { }

    /**
     * @brief Parses the command-line arguments.
     *
     * @param argc The argument count.
     * @param argv The argument values.
     * @return The result of the parsing.
     */
    ParseResult parse(std::size_t argc, const char** argv);

private:
    Args m_args;
    Cmds m_subcommands;

    ArgumentsBase const* m_current_args = &m_args;
    CommandsBase const* m_current_cms   = &m_subcommands;

    Ctx& m_ctx;
    std::span<const char*> m_values;
    std::size_t m_offset = 0;

    /**
     * @brief Parses a long option.
     *
     * @param opt The long option.
     * @return The result of the parsing.
     */
    ParseResult parse_long_opt(std::string_view opt);

    /**
     * @brief Parses a short option.
     *
     * @param opt The short option.
     * @return The result of the parsing.
     */
    ParseResult parse_short_opt(std::string_view opt);

    /**
     * @brief Parses a command or argument.
     *
     * @param opt The command or argument.
     * @return The result of the parsing.
     */
    ParseResult parse_command_or_argument(std::string_view opt);
};

/**
 * @brief Creates a Parser object.
 *
 * @tparam Ctx The context type.
 * @tparam UParser The user-defined parser type.
 * @tparam Args The argument types.
 * @tparam Cmds The command types.
 * @param ctx The context.
 * @param args The arguments.
 * @param commands The commands.
 * @return A Parser object.
 */
template <typename Ctx, parser<Ctx> UParser, are_arguments Args, are_commands Cmds>
constexpr auto make_parser(Ctx& ctx, Args&& args, Cmds&& commands) {
    return Parser<Ctx, UParser, Args, Cmds>(
        ctx, std::forward<decltype(args)>(args), std::forward<decltype(commands)>(commands)
    );
}

/**
 * @brief Processes the command-line arguments.
 *
 * @tparam Ctx The context type.
 * @tparam UParser The user-defined parser type.
 * @tparam Args The argument types.
 * @tparam Cmds The command types.
 * @param argc The argument count.
 * @param argv The argument values.
 * @param ctx The context.
 * @param args The arguments.
 * @param commands The commands.
 * @return The result of the parsing.
 */
template <typename Ctx, parser<Ctx> UParser, are_arguments Args, are_commands Cmds>
ParseResult process_args(std::size_t argc, const char** argv, Ctx& ctx, Args&& args, Cmds&& commands) {
    return Parser<Ctx, UParser, Args, Cmds>(
               ctx, std::forward<decltype(args)>(args), std::forward<decltype(commands)>(commands)
    )
        .parse(argc, argv);
}

template <typename Ctx, parser<Ctx> UParser, are_arguments Args, are_commands Cmds>
ParseResult Parser<Ctx, UParser, Args, Cmds>::parse(std::size_t argc, const char** argv) {
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

template <typename Ctx, parser<Ctx> UParser, are_arguments Args, are_commands Cmds>
ParseResult Parser<Ctx, UParser, Args, Cmds>::parse_short_opt(std::string_view opt) {

    if (opt.empty()) {
        return ParseResult::ERR_EMPTY_OPTION;
    }

    while (!opt.empty()) {
        const auto* arg_res = m_current_args->find_short(opt.front());
        if (arg_res == nullptr) {
            return ParseResult::ERR_UNKNOWN;
        }

        const auto& arg = *arg_res;
        auto result     = UParser::parse_option_flag(m_ctx, arg);
        if (result != ParseResult::OK) {
            return result;
        }

        opt = opt.substr(1);
    }

    return ParseResult::OK;
}

template <typename Ctx, parser<Ctx> UParser, are_arguments Args, are_commands Cmds>
ParseResult Parser<Ctx, UParser, Args, Cmds>::parse_long_opt(std::string_view opt) {

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

    const auto* arg_res = m_current_args->find_long(option);
    if (arg_res == nullptr) {
        return ParseResult::ERR_UNKNOWN;
    }

    const auto& arg = *arg_res;

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

template <typename Ctx, parser<Ctx> UParser, are_arguments Args, are_commands Cmds>
ParseResult Parser<Ctx, UParser, Args, Cmds>::parse_command_or_argument(std::string_view opt) {
    if (opt.empty()) {
        return ParseResult::OK;
    } else if (m_current_cms->size() == 0) {
        return UParser::parse_argument(m_ctx, opt);
    }

    const auto* cmd = m_current_cms->find(opt);
    if (cmd == nullptr) {
        return ParseResult::ERR_UNKNOWN;
    }

    m_current_args = &cmd->get_args();
    m_current_cms  = &cmd->get_cmds();

    return UParser::parse_command(m_ctx, *cmd);
}

}

#endif
