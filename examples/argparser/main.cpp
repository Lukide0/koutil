#include <iostream>
#include <koutil/argparser.h>
#include <string_view>
#include <vector>

using namespace koutil::argparser;
using namespace koutil::type;

constexpr auto a     = Arg::option_flag('a', "");
constexpr auto b     = Arg::option_flag('b', "");
constexpr auto exe   = make_subcommand("exe");
constexpr auto lib   = make_subcommand("lib");
constexpr auto build = make_subcommand("build", {}, Commands(exe, lib));
constexpr auto cmds  = Commands(build);

struct MyCtx {
    bool a = false;
    bool b = false;

    enum class Command {
        NONE,
        BUILD_EXE,
        BUILD_LIB,
    };

    Command cmd = Command::NONE;

    void print(std::ostream& stream) const {
        std::string_view cmd_name;

        switch (cmd) {
        case Command::NONE:
            cmd_name = "NONE";
            break;
        case Command::BUILD_EXE:
            cmd_name = "BUILD_EXE";
            break;
        case Command::BUILD_LIB:
            cmd_name = "BUILD_LIB";
            break;
        }

        stream << "MyCtx{ .a = " << a << ", .b = " << b << ", .cmd = " << cmd_name << "}\n";
    }
};

struct MyParser {
    static constexpr ParseResult parse_argument(MyCtx& /*unused*/, std::string_view /*unused*/) {
        return ParseResult::ERR_UNKNOWN;
    }

    static constexpr ParseResult parse_option_flag(MyCtx& ctx, const Arg& arg) {
        if (arg == 'a') {
            ctx.a = true;
        } else if (arg == 'b') {
            ctx.b = true;
        }
        return ParseResult::OK;
    }

    static constexpr ParseResult
    parse_option_value(MyCtx& /*unused*/, const Arg& /*unused*/, std::string_view /*unused*/) {
        return ParseResult::ERR_UNKNOWN;
    }

    static constexpr ParseResult parse_command(MyCtx& ctx, const SubcommandBase& cmd) {
        if (cmd == "exe") {
            ctx.cmd = MyCtx::Command::BUILD_EXE;
        } else if (cmd == "lib") {
            ctx.cmd = MyCtx::Command::BUILD_LIB;
        }

        return ParseResult::OK;
    }
};

int main(int arc, const char** argv) {
    MyCtx ctx;

    auto parser = make_parser<MyCtx, MyParser>(ctx, Arguments(a, b), cmds);
    auto result = parser.parse(arc, argv);

    switch (result) {
    case ParseResult::OK:
        break;
    case ParseResult::ERR:
        std::cout << "ERR\n";
        break;
    case ParseResult::ERR_EMPTY_OPTION:
        std::cout << "ERR EMPTY OPTION\n";
        break;
    case ParseResult::ERR_UNKNOWN:
        std::cout << "ERR UNKNOWN\n";
        break;
    case ParseResult::ERR_INVALID_VALUE:
        std::cout << "ERR INVALID VALUE\n";
        break;
    case ParseResult::ERR_MISSING_VALUE:
        std::cout << "ERR MISSING VALUE\n";
        break;
    }

    ctx.print(std::cout);
}
