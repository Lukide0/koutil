#ifndef KOUTIL_ARGS_OPTION_H
#define KOUTIL_ARGS_OPTION_H

#include "koutil/args/handle.h"
#include "koutil/args/result.h"
#include <cassert>
#include <functional>
#include <optional>
#include <string_view>
#include <utility>

namespace koutil::args {

/**
 * @brief Holds metadata for an option.
 *
 * @details Describes option names, description, value name, and flags such as
 * whether the option requires a value or is mandatory.
 * The order in which options are added affects their order in the help text,
 * but not their functionality.
 */
struct option_data_t {
    std::optional<std::string_view> long_name; ///< Long form name (without "--").
    std::optional<char> short_name; ///< Short form name (without "-").
    std::string_view description; ///< Description of the option.
    std::string_view value_name = "value"; ///< Name used to represent the option value.
    bool has_value              = false; ///< True if the option expects a value.
    bool required               = false; ///< True if the option is mandatory.
};

/**
 * @brief Represents a command-line option.
 *
 * @details Each option may have a short and/or long name, a description,
 * and an optional handler function. Options can be required or optional,
 * and may accept a value depending on configuration.
 *
 * The order in which options are added defines their order in the help text,
 * but does not affect command-line parsing behavior.
 */
template <extends_result Result = result_base_t> class option_t {
public:
    using result_t = Result;
    /// @brief Type alias for option handler function.
    using handle_t = std::function<void(std::optional<std::string_view>, result_t&)>;

    /**
     * @brief Constructs an option from data and a handler.
     * @tparam Handle Callable type compatible with `void(std::optional<std::string_view>, result_t&)`.
     * @param data Option metadata.
     * @param handle Function to process the option value.
     */
    template <void_handle<std::optional<std::string_view>, result_t&> Handle>
    explicit option_t(const option_data_t& data, Handle&& handle)
        : m_has_value(data.has_value)
        , m_required(data.required)
        , m_long_name(data.long_name)
        , m_short_name(data.short_name)
        , m_description(data.description)
        , m_value_name(data.value_name)
        , m_handle(std::forward<Handle>(handle)) { }

    /**
     * @brief Checks if the option is required.
     * @return True if the option is required.
     */
    [[nodiscard]] bool required() const { return m_required; }

    /**
     * @brief Checks if the option has been used.
     * @return True if the option was used during parsing.
     */
    [[nodiscard]] bool used() const { return m_used; }

    /**
     * @brief Gets the long option name.
     * @return Long name (without "--"), if present.
     */
    [[nodiscard]] auto long_name() const { return m_long_name; }

    /**
     * @brief Gets the short option name.
     * @return Short name (without "-"), if present.
     */
    [[nodiscard]] auto short_name() const { return m_short_name; }

    /**
     * @brief Gets the option description.
     * @return Option description.
     */
    [[nodiscard]] std::string_view description() const { return m_description; }

    /**
     * @brief Checks if the option expects a value.
     * @return True if the option expects a value.
     */
    [[nodiscard]] bool has_value() const { return m_has_value; }

    /**
     * @brief Gets the display name for the option value.
     * @return Option value name.
     */
    [[nodiscard]] std::string_view value_name() const { return m_value_name; }

    /**
     * @brief Processes the option value using the assigned handler.
     * @param value Option value, if provided.
     * @param result Result object to update.
     */
    void process(std::optional<std::string_view> value, result_t& result);

    /**
     * @brief Clears state
     */
    void clear_used() { m_used = false; }

private:
    bool m_has_value;
    bool m_required;

    bool m_used = false;

    std::optional<std::string_view> m_long_name;
    std::optional<char> m_short_name;
    std::string_view m_description;
    std::string_view m_value_name;

    handle_t m_handle;
};

template <extends_result Result>
void option_t<Result>::process(std::optional<std::string_view> value, result_t& result) {
    m_used = true;

    if (m_handle) {
        m_handle(value, result);
    }
}

}

#endif
