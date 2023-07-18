include(GoogleTest)
add_executable(TestXmlUtilities
        TestXmlUtilities.cpp)
target_include_directories(TestXmlUtilities SYSTEM BEFORE PUBLIC ${GTEST_INCLUDE} ${GMOCK_INCLUDE})
target_link_libraries(TestXmlUtilities ${GTEST_MAIN_LIBRARY} testhelpers xmlutilities)
target_link_libraries(TestXmlUtilities)
gtest_discover_tests(TestXmlUtilities)
SET_TARGET_PROPERTIES(TestXmlUtilities
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )