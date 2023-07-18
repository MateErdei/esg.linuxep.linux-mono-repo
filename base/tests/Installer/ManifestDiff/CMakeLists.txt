include(GoogleTest)
add_executable(TestManifestDiff
        TestManifest.cpp
        TestManifestEntry.cpp TestManifestDiff.cpp ExampleManifests.h TestCommandLine.cpp)
target_include_directories(TestManifestDiff SYSTEM BEFORE PUBLIC "${GTEST_INCLUDE}" "${GMOCK_INCLUDE}")
target_include_directories(TestManifestDiff BEFORE PUBLIC "${BOOST_INCLUDE}" "${CMAKE_SOURCE_DIR}")

SET_TARGET_PROPERTIES(TestManifestDiff
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

target_link_libraries(TestManifestDiff ${GTEST_MAIN_LIBRARY})
target_link_libraries(TestManifestDiff manifestdifflib testhelpers)
gtest_discover_tests(TestManifestDiff)
