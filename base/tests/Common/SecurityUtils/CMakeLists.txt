include(GoogleTest)
add_executable(TestSecurityUtils
        TestSecurityUtils.cpp)
target_include_directories(TestSecurityUtils SYSTEM BEFORE PUBLIC "${GTEST_INCLUDE}" "${GMOCK_INCLUDE}" ${CMAKE_SOURCE_DIR})
target_link_libraries(TestSecurityUtils ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY} ${GMOCK_LIBRARY} testhelpers securityutils)
target_link_libraries(TestSecurityUtils)
gtest_discover_tests(TestSecurityUtils)
SET_TARGET_PROPERTIES(TestSecurityUtils
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )