#ifndef KOUTIL_ARGPARSER_SUBCOMMAND_H
#define KOUTIL_ARGPARSER_SUBCOMMAND_H

#include "koutil/argparser/arg.h"
#include "koutil/type/types.h"
#include <cassert>
#include <concepts>
#include <cstddef>
#include <string_view>
#include <tuple>
#include <type_traits>

namespace koutil::argparser {

class CommandsBase;

/**
 * @brief Base class for subcommands.
 */
struct SubcommandBase {
    std::string_view name;

    constexpr SubcommandBase(std::string_view cmd_name)
        : name(cmd_name) { }

    constexpr virtual ~SubcommandBase() = default;

    /**
     * @brief Gets the arguments of the subcommand.
     *
     * @return A reference to the arguments of the subcommand.
     */
    [[nodiscard]] constexpr virtual const ArgumentsBase& get_args() const = 0;

    /**
     * @brief Gets the commands of the subcommand.
     *
     * @return A reference to the commands of the subcommand.
     */
    [[nodiscard]] constexpr virtual const CommandsBase& get_cmds() const = 0;

    constexpr bool operator==(std::string_view n) const { return name == n; }
};

/**
 * @brief Concept to check if a type satisfies the requirements of a subcommand.
 *
 * @tparam T The type to check.
 */
template <typename T>
concept is_subcommand = requires(T t) {
    std::is_base_of_v<SubcommandBase, T>;
    requires std::same_as<std::decay_t<decltype(t.name)>, std::string_view>;
};

/**
 * @brief Concept to check if a type satisfies the requirements of a collection of commands.
 *
 * @tparam T The type to check.
 */
template <typename T>
concept are_commands = std::is_base_of_v<CommandsBase, std::decay_t<T>>;

/**
 * @brief Base class for commands.
 */
class CommandsBase {
public:
    constexpr virtual ~CommandsBase() = default;

    /**
     * @brief Finds a subcommand by name.
     *
     * @param name The name of the subcommand to find.
     * @return A pointer to the found subcommand, or nullptr if not found.
     */
    [[nodiscard]] constexpr virtual const SubcommandBase* find(std::string_view name) const = 0;

    /**
     * @brief Gets the number of subcommands.
     *
     * @return The number of subcommands.
     */
    [[nodiscard]] constexpr virtual std::size_t size() const = 0;
};

/**
 * @brief Collection of subcommands.
 *
 * @tparam Cmd Types of subcommands.
 */
template <is_subcommand... Cmd> class Commands : public CommandsBase {
private:
    static constexpr std::size_t count = sizeof...(Cmd);
    using storage_t
        = std::conditional_t<count != 1, std::tuple<Cmd...>, type::types_get_t<type::types<std::decay_t<Cmd>...>, 0>>;

public:
    constexpr Commands(const Cmd&... cmd)
        requires(count != 1)
        : m_cmds(std::tuple<Cmd...>(cmd...)) {
        assert(!contains_duplicate());
    }

    /**
     * @brief Gets the number of subcommands.
     *
     * @return The number of subcommands.
     */
    [[nodiscard]] constexpr std::size_t size() const override { return count; }

    /**
     * @brief Finds a subcommand by name.
     *
     * @param name The name of the subcommand to find.
     * @return A pointer to the found subcommand, or nullptr if not found.
     */
    [[nodiscard]] constexpr const SubcommandBase* find(std::string_view name) const override {

        if constexpr (count == 1) {
            return &m_cmds;
        } else if constexpr (count == 0) {
            return nullptr;
        } else {
            return find_impl<0>(name);
        }
    }

private:
    storage_t m_cmds;

    /**
     * @brief Checks for duplicate subcommand names.
     *
     * @return True if duplicate names are found, false otherwise.
     */
    [[nodiscard]] constexpr bool contains_duplicate() const {
        if constexpr (count <= 1) {
            return false;
        }
        return contains_duplicate_impl<0, 1>();
    }

    /**
     * @brief Checks for duplicate subcommand names recursively.*
     * @tparam I Index of the current subcommand.
     * @tparam J Index of the subcommand to compare against.
     * @return True if duplicate names are found, false otherwise.
     */
    template <std::size_t I, std::size_t J>
        requires(I < count && J < count)
    [[nodiscard]] constexpr bool contains_duplicate_impl() const {
        const auto& cmd   = std::get<I>(m_cmds);
        const auto& other = std::get<J>(m_cmds);

        if (cmd.name == other.name) {
            return true;
        }

        if constexpr (J + 1 >= count) {
            return contains_duplicate_impl<I + 1, I + 2>();
        } else {
            return contains_duplicate_impl<I, J + 1>();
        }
    }

    template <std::size_t I, std::size_t J>
        requires(I >= count || J >= count)
    [[nodiscard]] constexpr bool contains_duplicate_impl() const {
        return false;
    }

    /**
     * @brief Finds a subcommand by name recursively.
     *
     * @tparam I Index of the current subcommand.
     * @param name The name of the subcommand to find.
     * @return A pointer to the found subcommand, or nullptr if not found.
     */
    template <std::size_t I> [[nodiscard]] constexpr const SubcommandBase* find_impl(std::string_view name) const {
        if constexpr (I >= count) {
            return nullptr;
        } else {

            const auto& cmd = std::get<I>(m_cmds);

            if (cmd.name == name) {
                return &cmd;
            } else {
                return find_impl<I + 1>(name);
            }
        }
    }
};

/**
 * @brief Specialization of Commands for a single subcommand.
 *
 * @tparam Cmd The type of the subcommand.
 */
template <is_subcommand Cmd> class Commands<Cmd> : public CommandsBase {
public:
    constexpr Commands(const Cmd& cmd)
        : m_cmd(cmd) { }

    [[nodiscard]] constexpr const SubcommandBase* find(std::string_view name) const override {
        if (m_cmd.name == name) {
            return &m_cmd;
        } else {
            return nullptr;
        }
    }

    [[nodiscard]] constexpr std::size_t size() const override { return 1; }

private:
    Cmd m_cmd;
};

/**
 * @brief Subcommand template struct.
 *
 * @tparam Args Argument types.
 * @tparam Cmd Subcommand types.
 */
template <typename Args, typename Cmd> struct Subcommand;

/**
 * @brief Specialization of Subcommand for argument and subcommand types.
 *
 * @tparam Args Argument types.
 * @tparam Cmd Subcommand types.
 */
template <is_arg... Args, is_subcommand... Cmd>
struct Subcommand<type::types<Args...>, type::types<Cmd...>> : public SubcommandBase {

    Arguments<Args...> args;
    Commands<Cmd...> cmds;

    /**
     * @brief Constructs a Subcommand object with the given name, arguments, and subcommands.
     *
     * @param cmd_name The name of the subcommand.
     * @param arguments The arguments of the subcommand.
     * @param commands The subcommands of the subcommand.
     */
    constexpr Subcommand(
        std::string_view cmd_name, const Arguments<Args...>& arguments, const Commands<Cmd...>& commands
    )
        : SubcommandBase(cmd_name)
        , args(arguments)
        , cmds(commands) { }

    /**
     * @brief Gets the arguments of the subcommand.
     *
     * @return A reference to the arguments of the subcommand.
     */
    [[nodiscard]] constexpr const ArgumentsBase& get_args() const override { return args; }

    /**
     * @brief Gets the commands of the subcommand.
     *
     * @return A reference to the commands of the subcommand.
     */
    [[nodiscard]] constexpr const CommandsBase& get_cmds() const override { return cmds; }
};

/**
 * @brief Helper function to create a subcommand.
 *
 * @tparam Args Argument types.
 * @tparam Cmds Subcommand types.
 * @param name The name of the subcommand.
 * @param args The arguments of the subcommand.
 * @param cmds The subcommands of the subcommand.
 * @return A Subcommand object.
 */
template <is_arg... Args, is_subcommand... Cmds>
constexpr auto make_subcommand(
    std::string_view name,
    const Arguments<Args...>& args = Arguments<> {},
    const Commands<Cmds...>& cmds  = Commands<> {}
) {
    return Subcommand<type::types<Args...>, type::types<Cmds...>>(name, args, cmds);
}

}

#endif
