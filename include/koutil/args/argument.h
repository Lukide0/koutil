#ifndef KOUTIL_ARGS_ARGUMENT_H
#define KOUTIL_ARGS_ARGUMENT_H

#include "koutil/args/handle.h"
#include "koutil/args/result.h"
#include <functional>
#include <string_view>

namespace koutil::args {

/**
 * @brief Represents a positional command-line argument.
 *
 * @details Each argument has a name, description, and an optional handler function.
 * The order in which arguments are added defines the order in which they are expected
 * on the command line and displayed in the help text.
 */
template <extends_result Result = result_base_t> class argument_t {
public:
    using result_t = Result;

    /// @brief Type alias for argument handler function.
    using handle_t = std::function<void(std::string_view, result_t&)>;

    /**
     * @brief Constructs an argument with a name and description.
     * @param name Argument name.
     * @param description Argument description.
     */
    explicit argument_t(std::string_view name, std::string_view description)
        : m_name(name)
        , m_description(description)
        , m_handle(nullptr) { }

    /**
     * @brief Constructs an argument with a name, description, and handler.
     * @tparam Handle Callable type compatible with `void(std::string_view, result_t&)`.
     * @param name Argument name.
     * @param description Argument description.
     * @param handle Function to process the argument value.
     */
    template <void_handle<std::string_view, result_t&> Handle>
    explicit argument_t(std::string_view name, std::string_view description, Handle&& handle)
        : m_name(name)
        , m_description(description)
        , m_handle(std::forward<Handle>(handle)) { }

    /**
     * @brief Gets the argument name.
     * @return Argument name.
     */
    [[nodiscard]] std::string_view name() const { return m_name; }

    /**
     * @brief Gets the argument description.
     * @return Argument description.
     */
    [[nodiscard]] std::string_view description() const { return m_description; }

    /**
     * @brief Processes the argument value using the assigned handler.
     * @param value Argument value.
     * @param result Result object to update.
     */
    void process(std::string_view value, result_t& result) const;

private:
    std::string_view m_name;
    std::string_view m_description;

    handle_t m_handle;
};

template <extends_result Result> void argument_t<Result>::process(std::string_view value, result_t& result) const {
    if (m_handle) {
        m_handle(value, result);
    }
}

}

#endif
