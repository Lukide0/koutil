#include "koutil/args/parser.h"
#include <charconv>
#include <cstdint>
#include <filesystem>
#include <format>
#include <iostream>

#include <koutil/args.h>
#include <koutil/term.h>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

using namespace koutil;

struct image_t {
    std::uint32_t width;
    std::uint32_t height;
    std::string output;
};

// Intermediate result structure for argument parsing
// Extends result_base_t to inherit error handling capabilities
struct my_result_t : args::result_base_t {
    std::optional<std::uint32_t> width;
    std::optional<std::uint32_t> height;
    std::string output;
};

// Helper function to convert string input to uint32_t with comprehensive validation
std::uint32_t convert_uint32(std::optional<std::string_view> value, my_result_t& res) {
    if (!value || value->empty()) {
        res.add_error("Value is required");
        return 0;
    }

    if ((*value)[0] == '-') {
        res.add_error(std::format("Value '{}' must be positive", *value));
        return 0;
    }

    std::uint32_t result = 0;
    auto [ptr, ec]       = std::from_chars(value->data(), value->data() + value->size(), result);

    if (ec == std::errc::invalid_argument) {
        res.add_error(std::format("Invalid value '{}': not a valid number", *value));
        return 0;
    }

    if (ec == std::errc::result_out_of_range) {
        res.add_error(
            std::format(
                "Invalid value '{}': number too large (max: {})", *value, std::numeric_limits<std::uint32_t>::max()
            )
        );
        return 0;
    }

    if (ptr != value->data() + value->size()) {
        res.add_error(std::format("Invalid value '{}': contains non-numeric characters", *value));
        return 0;
    }

    return result;
}

int main(int argc, char** argv) {
    // Get terminal dimensions for proper help text wrapping
    term::dimensions_t dims = term::terminal::query_dimensions();

    // Create argument parser with program name and description
    args::parser_t<my_result_t> parser("program", "description");

    // Add --help option to display usage information
    parser.add_option(
        args::option_builder_t<my_result_t>("help")
            .description("Show this text")
            .build([&]([[maybe_unused]] auto _, my_result_t& res) -> void {
                parser.show_help(std::cout, dims.width);
                res.exit(); // Signal that program should exit after showing help
            })
    );

    // Add -w/--width option (required, takes a value)
    parser.add_option(
        args::option_builder_t<my_result_t>('w', "width")
            .description("Image width")
            .required() // User must provide this option
            .has_value() // Option requires a value
            .build([](std::optional<std::string_view> value, my_result_t& res) -> void {
                // Prevent setting width multiple times
                if (res.width.has_value()) {
                    res.add_error("cannot set width: value already provided");
                    return;
                }
                // Parse and validate the width value
                res.width = convert_uint32(value, res);
            })
    );

    // Add -h/--height option (required, takes a value)
    parser.add_option(
        args::option_builder_t<my_result_t>('h', "height")
            .description("Image height")
            .required() // User must provide this option
            .has_value() // Option requires a value
            .build([](std::optional<std::string_view> value, my_result_t& res) -> void {
                // Prevent setting height multiple times
                if (res.height.has_value()) {
                    res.add_error("cannot set height: value already provided");
                    return;
                }
                // Parse and validate the height value
                res.height = convert_uint32(value, res);
            })
    );

    // Add positional argument for output file path
    parser.add_argument(args::argument_t<my_result_t>("output", "Output file", [](auto path, my_result_t& res) {
        if (std::filesystem::exists(path)) {
            res.add_error(std::format("'{}': already exists", path));
            return;
        }

        res.output = path;
    }));

    // Parse command line arguments (skip program name at argv[0])
    my_result_t result = parser.parse(argv + 1, argc - 1);

    // Check if parsing encountered any errors
    if (result.has_error()) {
        // Print all collected errors
        for (auto&& err : result.errors()) {
            std::cerr << "error: " << err << std::endl;
        }

        return 1;
    }

    // Check if program should exit early (e.g., --help was used)
    if (result.need_exit()) {
        return 0;
    }

    // All validation passed, create the final image structure
    // Use designated initializers for clarity
    image_t img {
        .width  = *result.width, // Safe to dereference - validated as present
        .height = *result.height, // Safe to dereference - validated as present
        .output = result.output,
    };

    // Display the parsed configuration
    std::cout << std::format("Image[{}x{}]: {}", img.width, img.height, img.output) << std::endl;

    return 0;
}
