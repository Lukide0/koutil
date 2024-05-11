#ifndef KOUTIL_UTIL_UTILS_H
#define KOUTIL_UTIL_UTILS_H

#if defined(__linux__)
    #define OS_LINUX
#elif defined(_WIN32) || defined(_WIN64)
    #define OS_WINDOWS
#endif

#include <cstdlib>
#include <string_view>

#ifdef OS_WINDOWS
    // [C4996] 'getenv': This function or variable may be unsafe
    #pragma warning(disable : 4996)
#endif

namespace koutil::util {

constexpr char ESC = '\x1B';

inline std::string_view safe_getenv(const char* name) {
    const char* value = std::getenv(name);
    if (value == nullptr) {
        return "";
    } else {
        return value;
    }
}

}

#endif
