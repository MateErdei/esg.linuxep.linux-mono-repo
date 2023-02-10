include(GoogleTest)
add_executable(TestUtilitiesImpl
        TestUtilities.cpp
        )
target_include_directories(TestUtilitiesImpl SYSTEM BEFORE PUBLIC ${GTEST_INCLUDE} ${GMOCK_INCLUDE})
target_include_directories(TestUtilitiesImpl BEFORE PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(TestUtilitiesImpl ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY})
target_link_libraries(TestUtilitiesImpl managementagentutilityimpl testhelpers )

gtest_discover_tests(TestUtilitiesImpl)
SET_TARGET_PROPERTIES(TestUtilitiesImpl
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )