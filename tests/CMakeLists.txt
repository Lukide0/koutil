include("${PROJECT_SOURCE_DIR}/cmake/doctest.cmake")

create_test("color" FILES color.test.cpp LIBS "${PROJECT_NAME}")
create_test("multi_vec" FILES multi_vec.test.cpp LIBS "${PROJECT_NAME}")
