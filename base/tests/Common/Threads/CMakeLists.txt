include(GoogleTest)
function(addTest TEST)
    add_executable(${TEST} ${TEST}.cpp)
    target_include_directories(${TEST} SYSTEM BEFORE PUBLIC ${GTEST_INCLUDE})
    target_link_libraries(${TEST} ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY} threads pthread)
    gtest_discover_tests(${TEST})
endfunction()

addTest(TestNotifyPipe)

SET_TARGET_PROPERTIES(TestNotifyPipe
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
