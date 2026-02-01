cmake_minimum_required(VERSION 3.22)
project(playground)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wpedantic")

# include(cmake/dependencies_package.cmake)
# add_library(foo_impl b.cpp)
# add_executable(main main.cpp)
# target_link_libraries(main foo_impl)
add_subdirectory(src)