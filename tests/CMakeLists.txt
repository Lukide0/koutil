include("${PROJECT_SOURCE_DIR}/cmake/doctest.cmake")

create_test("color" FILES color.test.cpp LIBS "${PROJECT_NAME}")
