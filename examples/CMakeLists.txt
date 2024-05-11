function(create_example name)
    set(prefix ARG)
    set(flags "")
    set(one_value_options "")
    set(multiple_value_options FILES INCLUDE LIBS)

    cmake_parse_arguments(PARSE_ARGV 1 ${prefix} "${flags}"
                        "${one_value_options}" "${multiple_value_options}")

    add_executable("${name}" ${ARG_FILES})

    target_compile_features("${name}" PRIVATE cxx_std_23)
    set_target_properties("${name}" PROPERTIES CXX_EXTENSIONS OFF)

    target_include_directories("${name}" PRIVATE "${ARG_INCLUDE}")
    target_link_libraries("${name}" ${ARG_LIBS})

endfunction()

create_example("colors" FILES colors/main.cpp LIBS "${PROJECT_NAME}")
create_example("styles" FILES styles/main.cpp LIBS "${PROJECT_NAME}")
create_example("commands" FILES commands/main.cpp LIBS "${PROJECT_NAME}")
