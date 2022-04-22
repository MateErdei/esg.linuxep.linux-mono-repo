include(GoogleTest)
add_executable(TestManifestDiff
        TestManifest.cpp
        TestManifestEntry.cpp TestManifestDiff.cpp ExampleManifests.h TestCommandLine.cpp)
target_include_directories(TestManifestDiff SYSTEM BEFORE PUBLIC "${GTEST_INCLUDE}" "${GMOCK_INCLUDE}")
target_include_directories(TestManifestDiff BEFORE PUBLIC "${BOOST_INCLUDE}" "${CMAKE_SOURCE_DIR}" )

set (BOOST_LIB_DIRS "${BOOST_INPUT}/lib64")

SET_TARGET_PROPERTIES(TestManifestDiff
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

target_link_libraries(TestManifestDiff ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY} ${GMOCK_LIBRARY})
target_link_libraries(TestManifestDiff manifestdifflib testhelpers -L"${BOOST_LIB_DIRS}" -lboost_filesystem -lboost_system )
gtest_discover_tests(TestManifestDiff)
