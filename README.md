# koutil

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](./LICENSE)
![C++ Version](https://img.shields.io/badge/C%2B%2B-20-blue.svg)

koutil is a C++ header library providing utilities for terminal-based applications. It offers a collection of helpful functions and classes to simplify common tasks in terminal environments.

## Features

- **Colors**
- **Styles**
- **Cursor movement**

## Requirements

- **cmake**
- **c++20**

## Installation

```cmake
include(FetchContent)
find_package(Git REQUIRED)

FetchContent_Declare(
  koutil_repo
  GIT_REPOSITORY https://github.com/Lukide0/koutil.git
  GIT_TAG "##TAG##"
  GIT_SHALLOW TRUE
  GIT_PROGRESS ON)
FetchContent_MakeAvailable(koutil_repo)

add_subdirectory(${koutil_repo_SOURCE_DIR})

...

target_link_libraries(your_application <PRIVATE|PUBLIC|INTERFACE> koutil)
```

## Generated documentation

- [doxygen](https://lukide0.github.io/koutil/)
