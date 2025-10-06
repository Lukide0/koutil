#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <cstdlib>
#include <doctest/doctest.h>
#include <koutil/args.h>
#include <koutil/args/errors.h>
#include <koutil/container/comptime_map.h>
#include <span>
#include <string_view>
#include <vector>

using namespace koutil::args;

TEST_CASE("[ARGS][Arguments]") {
    std::vector<std::string_view> arguments;

    const auto args = std::to_array<const char*>({
        "0",
        "1",
        "2",
        "3",
        "4",
        "5",
        "6",
        "7",
        "8",
        "9",
    });

    constexpr std::size_t args_count = args.size();

    std::span<const char* const> all_args = args;

    parser_t<> parser("test", "");

    for (std::size_t i = 0; i < args_count; ++i) {
        parser.add_argument(argument_t<>("arg", "", [&](auto value, result_base_t& _) { arguments.push_back(value); }));
    }

    {
        const auto result = parser.parse(all_args);
        CHECK_FALSE(result.has_error());

        CHECK(std::ranges::equal(all_args, arguments));

        // prepare vector for the next test
        arguments.clear();
    }

    for (std::size_t i = 1; i <= args_count; ++i) {
        const auto result = parser.parse(all_args.subspan(i));
        CHECK(result.has_error());

        const auto& errs = result.errors();
        CHECK(errs.size() == 1);

        CHECK_EQ(errs.front(), error::make_argument_count(parser.name(), args_count, arguments.size()));

        CHECK_EQ(arguments.size(), args_count - i);
        arguments.clear();
    }
}

TEST_CASE("[ARGS][Terminator]") {
    std::vector<std::string_view> arguments;

    const auto args = std::to_array<const char*>({
        "--",
        "-0",
        "-1",
        "-2",
        "--"
        "-4",
        "-5",
        "-6",
        "-7",
        "-8",
        "-9",
    });

    constexpr std::size_t args_count = args.size() - 1;

    std::span<const char* const> all_args = args;

    parser_t<> parser("test", "");

    for (std::size_t i = 0; i < args_count; ++i) {
        parser.add_argument(argument_t<>("arg", "", [&](auto value, result_base_t& _) { arguments.push_back(value); }));
    }

    const auto result = parser.parse(all_args);
    CHECK_FALSE(result.has_error());

    // remove "--"
    CHECK(std::ranges::equal(all_args.subspan(1), arguments));
}

TEST_CASE("[ARGS][OPTION][SHORT]") {
    constexpr auto args = std::to_array<const char*>({
        "-a", "-b", "-c", "-d", "-e", "-f", "-g", "-h", "-i", "-j", "-k", "-l", "-m",
        "-n", "-o", "-p", "-q", "-r", "-s", "-t", "-u", "-v", "-w", "-x", "-y", "-z",
    });

    constexpr std::size_t args_count    = args.size();
    constexpr std::size_t options_count = 'z' - 'a' + 1;
    static_assert(options_count == args_count);

    std::span<const char* const> all_args = args;

    parser_t<> parser("test", "");

    std::bitset<options_count> flags = 0;

    for (std::size_t i = 0; i < options_count; ++i) {
        REQUIRE(parser.add_option(
            option_builder_t<>('a' + i).handle([&flags, i](auto _, result_base_t&) { flags.set(i); }).build()
        ));
    }

    const auto result = parser.parse(all_args);
    REQUIRE(!result.has_error());

    CHECK_EQ(options_count, flags.count());
}

TEST_CASE("[ARGS][OPTION][LONG]") {
    constexpr auto args = std::to_array<const char*>({
        "--a", "--b", "--c", "--d", "--e", "--f", "--g", "--h", "--i", "--j", "--k", "--l", "--m",
        "--n", "--o", "--p", "--q", "--r", "--s", "--t", "--u", "--v", "--w", "--x", "--y", "--z",
    });

    constexpr std::size_t args_count    = args.size();
    constexpr std::size_t options_count = 'z' - 'a' + 1;
    static_assert(options_count == args_count);

    std::span<const char* const> all_args = args;

    parser_t<> parser("test", "");

    std::bitset<options_count> flags = 0;

    for (std::size_t i = 0; i < options_count; ++i) {
        std::string_view name = args[i];

        // remove "--"
        name.remove_prefix(2);

        REQUIRE(parser.add_option(
            option_builder_t<>(name).handle([&flags, i](auto _, result_base_t&) { flags.set(i); }).build()
        ));
    }

    const auto result = parser.parse(all_args);
    REQUIRE(!result.has_error());

    CHECK_EQ(options_count, flags.count());
}

TEST_CASE("[ARGS][OPTION][LONG AND SHORT]") {
    constexpr auto args = std::to_array<const char*>({
        "--a", "-b",  "--c", "-d",  "--e", "-f",  "--g", "-h",  "--i", "-j",  "--k", "-l",  "--m",
        "-n",  "--o", "-p",  "--q", "-r",  "--s", "-t",  "--u", "-v",  "--w", "-x",  "--y", "-z",
    });

    constexpr std::size_t args_count    = args.size();
    constexpr std::size_t options_count = 'z' - 'a' + 1;
    static_assert(options_count == args_count);

    std::span<const char* const> all_args = args;

    parser_t<> parser("test", "");

    std::bitset<options_count> flags = 0;

    for (std::size_t i = 0; i < options_count; ++i) {
        std::string_view long_name = args[i];
        if (i % 2 == 0) {
            // remove "--"
            long_name.remove_prefix(2);
        } else {
            long_name.remove_prefix(1);
        }

        assert(long_name.size() == 1);

        char short_name = long_name[0];

        REQUIRE(parser.add_option(
            option_builder_t<>(short_name, long_name)
                .handle([&flags, i](auto _, result_base_t&) { flags.set(i); })
                .build()
        ));
    }

    const auto result = parser.parse(all_args);
    REQUIRE(!result.has_error());

    CHECK_EQ(options_count, flags.count());
}

TEST_CASE("[ARGS][OPTION][VALUE]") {
    std::string_view width;
    std::string_view height;

    parser_t<> parser("test", "");

    parser.add_option(option_builder_t<>("width").has_value().build([&](auto value, auto& _) { width = *value; }));
    parser.add_option(option_builder_t<>("height").has_value().build([&](auto value, auto& _) { height = *value; }));

    {
        constexpr auto args = std::to_array<const char*>({ "--width=5", "--height", "10" });

        const auto result = parser.parse(args);
        CHECK_FALSE(result.has_error());

        CHECK_EQ("5", width);
        CHECK_EQ("10", height);

        width  = "";
        height = "";
    }

    {
        constexpr auto args = std::to_array<const char*>({ "--width=5", "--height" });

        const auto result = parser.parse(args);
        CHECK(result.has_error());

        const auto& errors = result.errors();
        CHECK_EQ(errors.size(), 1);

        CHECK_EQ(errors.front(), error::make_option_requires_value("--height"));

        width  = "";
        height = "";
    }
}

TEST_CASE("[ARGS][OPTION][REQUIRED]") {

    std::string_view width;
    std::string_view height;

    parser_t<> parser("test", "");

    auto width_option
        = option_builder_t<>("width").has_value().required().build([&](auto value, auto& _) { width = *value; });

    auto height_option = option_builder_t<>("height").has_value().build([&](auto value, auto& _) { height = *value; });

    parser.add_option(width_option);
    parser.add_option(height_option);
    {

        constexpr auto args = std::to_array<const char*>({ "--width=5", "--height", "10" });

        const auto result = parser.parse(args);
        CHECK_FALSE(result.has_error());

        CHECK_EQ("5", width);
        CHECK_EQ("10", height);

        width  = "";
        height = "";
    }

    {
        parser.clear_used();

        constexpr auto args = std::to_array<const char*>({ "--width=5" });

        const auto result = parser.parse(args);
        CHECK_FALSE(result.has_error());

        CHECK_EQ("5", width);
        CHECK(height.empty());

        width  = "";
        height = "";
    }

    {
        parser.clear_used();

        constexpr auto args = std::to_array<const char*>({ "--height", "10" });

        const auto result = parser.parse(args);
        CHECK(result.has_error());

        const auto& errors = result.errors();
        CHECK_EQ(errors.size(), 1);

        CHECK_EQ(errors.front(), error::make_missing_required_option(width_option));

        width  = "";
        height = "";
    }
}

TEST_CASE("[ARGS][COMMAND]") {
    enum class CommandType {
        BUILD,
        BUILD_LIB,
        BUILD_EXE,
        TEST,
        NONE,
    };

    CommandType command = CommandType::NONE;

    parser_t<> parser("test", "");
    parser.add_command(
        command_builder_t<>("build", "", [&](auto, auto&) { command = CommandType::BUILD; })
            .add_command(command_builder_t<>("lib", "", [&](auto, auto&) { command = CommandType::BUILD_LIB; }).build())
            .add_command(command_builder_t<>("exe", "", [&](auto, auto&) { command = CommandType::BUILD_EXE; }).build())
            .build()
    );
    parser.add_command(command_builder_t<>("test", "", [&](auto, auto&) { command = CommandType::TEST; }).build());

    {
        constexpr auto args = std::to_array<const char*>({ "build" });

        const auto result = parser.parse(args);
        CHECK_FALSE(result.has_error());

        CHECK_EQ(CommandType::BUILD, command);

        command = CommandType::NONE;
    }

    {
        constexpr auto args = std::to_array<const char*>({ "test" });

        const auto result = parser.parse(args);
        CHECK_FALSE(result.has_error());

        CHECK_EQ(CommandType::TEST, command);

        command = CommandType::NONE;
    }

    {
        constexpr auto args = std::to_array<const char*>({ "build", "lib" });

        const auto result = parser.parse(args);
        CHECK_FALSE(result.has_error());

        CHECK_EQ(CommandType::BUILD_LIB, command);

        command = CommandType::NONE;
    }

    {
        constexpr auto args = std::to_array<const char*>({ "build", "exe" });

        const auto result = parser.parse(args);
        CHECK_FALSE(result.has_error());

        CHECK_EQ(CommandType::BUILD_EXE, command);

        command = CommandType::NONE;
    }

    {
        constexpr auto args = std::to_array<const char*>({ "build", "test" });

        const auto result = parser.parse(args);
        CHECK(result.has_error());

        const auto& errors = result.errors();
        CHECK_EQ(errors.size(), 1);

        CHECK_EQ(errors.front(), error::make_unknown_argument("test"));
    }

    {
        constexpr auto args = std::to_array<const char*>({ "test", "exe" });

        const auto result = parser.parse(args);
        CHECK(result.has_error());

        const auto& errors = result.errors();
        CHECK_EQ(errors.size(), 1);

        CHECK_EQ(errors.front(), error::make_unknown_argument("exe"));
    }
}
