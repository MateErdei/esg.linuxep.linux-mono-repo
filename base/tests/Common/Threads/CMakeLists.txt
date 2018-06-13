
function(addTest TEST)
    add_executable(${TEST} ${TEST}.cpp)
    target_include_directories(${TEST} SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include")
    target_include_directories(${TEST} BEFORE PUBLIC "${gtest_SOURCE_DIR}")
    target_link_libraries(${TEST} gtest gtest_main)
    target_link_libraries(${TEST} threads)
    add_test(NAME ${TEST} COMMAND ${TEST})
endfunction()

addTest(TestNotifyPipe)
