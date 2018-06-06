cmake_minimum_required(VERSION 3.10)
project(basetesting)

set(CMAKE_CXX_STANDARD 11)

enable_testing()

add_subdirectory(googletest)

add_executable(testtest testtest.cpp)
target_include_directories(testtest SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include")
target_include_directories(testtest BEFORE PUBLIC "${gtest_SOURCE_DIR}")

target_link_libraries(testtest gtest gtest_main)

add_custom_target(echogTest
        COMMAND echo gtest_SOURCE_DIR=${gtest_SOURCE_DIR}
        COMMAND ls -l ${gtest_SOURCE_DIR}/include/gtest/gtest.h
        )

