#ifndef KOUTIL_ARGPARSER_ARG_H
#define KOUTIL_ARGPARSER_ARG_H

#include <algorithm>
#include <array>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>

namespace koutil::experimental::argparser {

struct Arg;

template <typename T>
concept arg_type = std::is_same_v<Arg, std::decay_t<T>>;

struct Arg {
    enum Type {
        OPTION_FLAG,
        OPTION_VALUE,
    } type;

    const char short_name;
    const std::string_view long_name;
    const std::string_view description;

    static constexpr Arg option_flag(char opt, std::string_view desc) {
        assert(opt != '\0');
        return { OPTION_FLAG, opt, "", desc };
    }

    static constexpr Arg option_flag(std::string_view long_name, std::string_view desc) {
        assert(!long_name.empty());
        return { OPTION_FLAG, '\0', long_name, desc };
    }

    static constexpr Arg option_flag(char opt, std::string_view long_name, std::string_view desc) {
        assert(opt != '\0' && !long_name.empty());
        return { OPTION_FLAG, opt, long_name, desc };
    }

    static constexpr Arg option_value(std::string_view long_name, std::string_view desc) {
        assert(!long_name.empty());
        return { OPTION_VALUE, '\0', long_name, desc };
    }

    constexpr bool operator==(const Arg& other) const {
        return short_name == other.short_name && long_name == other.long_name && description == other.description;
    }

    constexpr bool operator<(char name) const { return short_name < name; }

    constexpr bool operator<(std::string_view name) const { return long_name < name; }

    constexpr bool operator>(char name) const { return short_name > name; }

    constexpr bool operator>(std::string_view name) const { return long_name > name; }

    constexpr bool operator==(char name) const { return short_name == name; }

    constexpr bool operator==(std::string_view name) const { return long_name == name; }

private:
    constexpr Arg(Type arg_type, char sname, std::string_view lname, std::string_view desc)
        : type(arg_type)
        , short_name(sname)
        , long_name(lname)
        , description(desc) { }
};

class ArgumentsBase {
public:
    constexpr virtual ~ArgumentsBase()                                                               = default;
    [[nodiscard]] constexpr virtual std::size_t size() const                                         = 0;
    [[nodiscard]] constexpr virtual std::size_t count_short_args() const                             = 0;
    [[nodiscard]] constexpr virtual std::size_t count_long_args() const                              = 0;
    [[nodiscard]] constexpr virtual std::optional<const Arg*> find_short(char name) const            = 0;
    [[nodiscard]] constexpr virtual std::optional<const Arg*> find_long(std::string_view name) const = 0;
    [[nodiscard]] constexpr virtual std::span<const Arg> args() const                                = 0;
};

template <typename T>
concept arguments = requires(T t, char c, std::string_view sv) {
    std::is_base_of_v<ArgumentsBase, T>;
    { t.size() } -> std::same_as<std::size_t>;
    { t.count_short_args() } -> std::same_as<std::size_t>;
    { t.count_long_args() } -> std::same_as<std::size_t>;
    { t.find_short(c) } -> std::same_as<std::optional<const Arg*>>;
    { t.find_long(sv) } -> std::same_as<std::optional<const Arg*>>;
};

template <arg_type... Args> class Arguments : public ArgumentsBase {
private:
    using args_t     = std::array<Arg, sizeof...(Args)>;
    using args_ref_t = std::pair<std::size_t, std::array<std::size_t, sizeof...(Args)>>;

public:
    constexpr Arguments(const Args&... args)
        : m_args({ (std::forward<decltype(args)>(args))... }) {
        create_refs();

        assert(!contains_duplicates(m_short_args, &Arg::short_name));
        assert(!contains_duplicates(m_long_args, &Arg::long_name));
    }

    ~Arguments() override = default;

    [[nodiscard]] constexpr std::size_t size() const override { return sizeof...(Args); }

    [[nodiscard]] constexpr std::size_t count_short_args() const override { return m_short_args.first; }

    [[nodiscard]] constexpr std::size_t count_long_args() const override { return m_long_args.first; }

    [[nodiscard]] constexpr std::optional<const Arg*> find_short(char name) const override {

        std::span view(m_short_args.second.begin(), m_short_args.first);
        for (auto i : view) {
            if (m_args[i].short_name == name) {
                return std::make_optional(&m_args[i]);
            }
        }

        return {};
    }

    [[nodiscard]] constexpr std::optional<const Arg*> find_long(std::string_view name) const override {

        std::span view(m_long_args.second.begin(), m_long_args.first);
        for (auto i : view) {
            if (m_args[i].long_name == name) {
                return std::make_optional(&m_args[i]);
            }
        }

        return {};
    }

    [[nodiscard]] constexpr std::span<const Arg> args() const override { return m_args; }

private:
    args_t m_args;
    args_ref_t m_short_args;
    args_ref_t m_long_args;

    constexpr void create_refs() {
        for (auto&& [i, arg] : m_args | std::views::enumerate) {

            if (arg.short_name != '\0') {
                m_short_args.second[m_short_args.first++] = i;
            } else if (!arg.long_name.empty()) {
                m_long_args.second[m_long_args.first++] = i;
            } else {
                assert(false);
            }
        }
    }

    template <typename Proj> constexpr bool contains_duplicates(const args_ref_t& search, Proj member) {
        std::span view(search.second.begin(), search.first);

        for (auto it = view.begin(); it != view.end(); ++it) {
            const auto& arg = m_args[*it];

            for (auto it2 = it + 1; it2 != view.end(); ++it2) {
                const auto& other = m_args[*it2];

                if (arg.*member == other.*member) {
                    return true;
                }
            }
        }

        return false;
    }
};

}

#endif
