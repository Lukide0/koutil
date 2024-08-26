include_guard(GLOBAL)

include(FetchContent)
find_package(Git REQUIRED)

FetchContent_Declare(
  doctest
  GIT_REPOSITORY https://github.com/doctest/doctest.git
  GIT_TAG "v2.4.11"
  GIT_SHALLOW TRUE
  GIT_PROGRESS ON)
FetchContent_MakeAvailable(doctest)

FetchContent_GetProperties(doctest)
if(NOT doctest_POPULATED)
    FetchContent_Populate(doctest)
endif()

set(DOCTEST_INCLUDE_DIR
    ${doctest_SOURCE_DIR}
    CACHE INTERNAL "Path to include folder for doctest")

function(create_test name)

    set(prefix ARG)
    set(multiple_value_options FILES LIBS INCLUDE)

    cmake_parse_arguments(PARSE_ARGV 1 ${prefix} "" ""
                        "${multiple_value_options}")

    add_executable("${name}" ${ARG_FILES})

    target_compile_features("${name}" PRIVATE cxx_std_20)
    set_target_properties("${name}" PROPERTIES CXX_EXTENSIONS OFF)

    target_include_directories("${name}" PRIVATE "${ARG_INCLUDE}"
                                               ${DOCTEST_INCLUDE_DIR})
    target_link_libraries("${name}" PRIVATE ${ARG_LIBS})

    target_compile_definitions("${name}"
                             PRIVATE DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN)

    target_compile_options("${name}" PRIVATE -fsanitize=address)
    target_link_options("${name}" PRIVATE -fsanitize=address)

    add_test(NAME "${name}" COMMAND "${name}")

endfunction()
