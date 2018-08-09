include(GoogleTest)
add_executable(TestXmlUtilities
        TestXmlUtilities.cpp)
target_include_directories(TestXmlUtilities SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(TestXmlUtilities BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${gmock_SOURCE_DIR}")
target_link_libraries(TestXmlUtilities gtest gtest_main gmock testhelpers xmlutilities)
target_link_libraries(TestXmlUtilities)
gtest_discover_tests(TestXmlUtilities)
