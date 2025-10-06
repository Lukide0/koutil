#ifndef KOUTIL_ARGS_ERRORS_H
#define KOUTIL_ARGS_ERRORS_H

#include "koutil/args/option.h"
#include "koutil/args/result.h"
#include <cstdint>
#include <format>
#include <string>
#include <string_view>

namespace koutil::args::error {

inline std::string make_argument_count(std::string_view name, std::uint32_t expected, std::uint32_t provided) {
    return std::format(
        "'{}' requires {} argument{}, but {} {} provided",
        name,
        expected,
        expected == 1 ? "" : "s",
        provided,
        provided == 1 ? "was" : "were"
    );
}

inline std::string make_command_argument_count(std::string_view name, std::uint32_t expected, std::uint32_t provided) {
    return "command " + make_argument_count(name, expected, provided);
}

inline std::string make_unknown_argument(std::string_view name) { return std::format("unknown argument: {}", name); }

inline std::string make_unknown_long_option(std::string_view name) {
    return std::format("unknown option '--{}'", name);
}

inline std::string make_unknown_short_option(char name) { return std::format("unknown option '-{}'", name); }

inline std::string make_invalid_short_option(std::string_view name) {
    return std::format("Invalid short option '-{}': must be a single character", name);
}

inline std::string make_option_requires_value(std::string_view whole_name) {
    return std::format("option '{}' requires a value", whole_name);
}

inline std::string make_unexpected_option_value(std::string_view whole_name) {
    return std::format("option '{}' doesn't accept a value", whole_name);
}

template <extends_result Result> inline std::string make_missing_required_option(const option_t<Result>& option) {
    // format option name
    std::string display;
    if (option.long_name()) {
        display = std::format("--{}", *option.long_name());
        if (option.short_name()) {
            display += std::format(" (-{})", *option.short_name());
        }
    } else if (option.short_name()) {
        display = std::format("-{}", *option.short_name());
    }

    return std::format("required option {} was not provided", display);
}

}

#endif
