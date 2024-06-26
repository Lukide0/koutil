cmake_minimum_required(VERSION 3.23)

project(
  koutil
  VERSION 0.1.0
  LANGUAGES CXX)

include(cmake/bootstrap.cmake)

# ------------------------------------------------------------------------------
# Set options

option(ENABLE_KOUTIL_TESTING "Build and run tests" ON)
option(ENABLE_KOUTIL_BUILD_EXAMPLES "Build examples" ON)

# ------------------------------------------------------------------------------
# Target

add_library(${PROJECT_NAME} INTERFACE)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)
target_include_directories(${PROJECT_NAME}
                           INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS OFF)

if(ENABLE_KOUTIL_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

if(ENABLE_KOUTIL_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
