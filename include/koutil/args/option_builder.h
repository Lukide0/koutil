#ifndef KOUTIL_ARGS_OPTION_BUILDER_H
#define KOUTIL_ARGS_OPTION_BUILDER_H

#include "koutil/args/handle.h"
#include "koutil/args/option.h"
#include "koutil/args/result.h"
#include <optional>
#include <string_view>
#include <utility>

namespace koutil::args {

/**
 * @brief Builder for creating command-line options.
 *
 * @details Provides an interface to construct an @ref option_t instance.
 * The builder allows setting names, description, flags, and a handler function.
 *
 * Example:
 * @code
 * auto opt = option_builder_t('o', "output")
 *     .description("Output file path")
 *     .has_value()
 *     .required()
 *     .handle([](auto value, result_t& r) {  ...  })
 *     .build();
 * @endcode
 */
template <extends_result Result = result_base_t> class option_builder_t {
public:
    using result_t = Result;
    using option_t = option_t<result_t>;

    /**
     * @brief Constructs a builder with both short and long names.
     * @param short_name Short name (without "-").
     * @param long_name Long name (without "--").
     */
    option_builder_t(char short_name, std::string_view long_name) {
        m_data.short_name = short_name;
        m_data.long_name  = long_name;
    }

    /**
     * @brief Constructs a builder with only a short name.
     * @param short_name Short name (without "-").
     */
    option_builder_t(char short_name) { m_data.short_name = short_name; }

    /**
     * @brief Constructs a builder with only a long name.
     * @param long_name Long name (without "--").
     */
    option_builder_t(std::string_view long_name) { m_data.long_name = long_name; }

    /**
     * @brief Sets the option description.
     * @param description Description text.
     * @return Builder.
     */
    option_builder_t&& description(std::string_view description) && {
        m_data.description = description;
        return std::move(*this);
    }

    /**
     * @brief Marks the option as required.
     * @param required True if required.
     * @return Builder.
     */
    option_builder_t&& required(bool required = true) && {
        m_data.required = required;
        return std::move(*this);
    }

    /**
     * @brief Marks the option as one that expects a value.
     * @param value True if it has a value.
     * @return Builder.
     */
    option_builder_t&& has_value(bool value = true) && {
        m_data.has_value = value;
        return std::move(*this);
    }

    /**
     * @brief Assigns a handler function to the option.
     * @tparam Handle Callable type compatible with `void(std::optional<std::string_view>, result_t&)`.
     * @param handle Function to handle the option’s value.
     * @return Builder.
     */
    template <void_handle<std::optional<std::string_view>, result_t&> Handle>
    option_builder_t&& handle(Handle&& handle) && {
        m_handle = std::forward<Handle>(handle);
        return std::move(*this);
    }

    /**
     * @brief Builds the option with the configured properties.
     * @return Constructed option.
     */
    option_t build() && { return option_t(m_data, std::move(m_handle)); }

    /**
     * @brief Builds the option with an explicit handler.
     * @tparam Handle Callable type compatible with `void(std::optional<std::string_view>, result_t&)`.
     * @param handle Function to handle the option’s value.
     * @return Constructed option.
     */
    template <void_handle<std::optional<std::string_view>, result_t&> Handle> option_t build(Handle&& handle) && {
        assert(m_handle == nullptr);
        return option_t(m_data, std::forward<Handle>(handle));
    }

private:
    option_data_t m_data;
    option_t::handle_t m_handle;
};

}

#endif
