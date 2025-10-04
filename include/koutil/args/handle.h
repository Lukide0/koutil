#ifndef KOUTIL_ARGS_HANDLE_H
#define KOUTIL_ARGS_HANDLE_H

#include <concepts>
#include <type_traits>

namespace koutil::args {

template <typename Fn, typename... Args>
concept void_handle = std::invocable<Fn, Args...> && std::is_void_v<std::invoke_result_t<Fn, Args...>>;

}

#endif
