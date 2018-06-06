
add_executable(TestContextHolder TestContextHolder.cpp)

target_include_directories(TestContextHolder SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include")
target_include_directories(TestContextHolder BEFORE PUBLIC "${gtest_SOURCE_DIR}")

target_link_libraries(TestContextHolder gtest gtest_main)

target_link_libraries(TestContextHolder zeromqwrapperimpl)

add_test(NAME TestContextHolder COMMAND TestContextHolder)
