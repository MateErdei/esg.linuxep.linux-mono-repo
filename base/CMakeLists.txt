cmake_minimum_required(VERSION 3.5)
project(everest_base CXX)

option(test "Build all tests." ON) # Makes boolean 'test' available.

set(CMAKE_CXX_STANDARD 11)

enable_testing()

find_package(PkgConfig)
FIND_PACKAGE ( Threads REQUIRED )

include_directories(modules)

add_subdirectory(modules)
add_subdirectory(products)
add_subdirectory(tests)
