include(GoogleTest)
add_subdirectory(googletest)

add_executable(testtest testtest.cpp)
target_include_directories(testtest SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include")
target_include_directories(testtest BEFORE PUBLIC "${gtest_SOURCE_DIR}")

target_link_libraries(testtest gtest gtest_main)

gtest_discover_tests(testtest)

add_custom_target(echogTest
        COMMAND echo gtest_SOURCE_DIR=${gtest_SOURCE_DIR}
        COMMAND ls -l ${gtest_SOURCE_DIR}/include/gtest/gtest.h
        )

add_subdirectory(Common)
add_subdirectory(SulDownloader)
