#ifndef KOUTIL_ARGPARSER_SUBCOMMAND_H
#define KOUTIL_ARGPARSER_SUBCOMMAND_H

#include "koutil/experimental/argparser/arg.h"
#include <concepts>
#include <string_view>

namespace koutil::experimental::argparser {

template <typename T>
concept subcommand = requires(T t) {
    requires std::same_as<std::decay_t<decltype(t.name)>, std::string_view>;
    requires arguments<decltype(t.args)>;
};

template <arg_type... Args> struct Subcommand {
    std::string_view name;
    Arguments<Args...> args;
};

template <arg_type... Args> constexpr auto make_subcommand(std::string_view name, Args&&... args) {
    return Subcommand { .name = name, .args = Arguments { std::forward<decltype(args)>(args)... } };
}

}

#endif
