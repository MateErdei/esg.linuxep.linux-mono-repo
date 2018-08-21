include(GoogleTest)
add_executable(TestManifestDiff
        TestManifest.cpp
        TestManifestEntry.cpp TestManifestDiff.cpp ExampleManifests.h TestCommandLine.cpp)
target_include_directories(TestManifestDiff SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(TestManifestDiff BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${gmock_SOURCE_DIR}" "${BOOST_INCLUDE}" "${CMAKE_SOURCE_DIR}" )

set (BOOST_LIB_DIRS "${BOOST_INPUT}/lib64")

SET_TARGET_PROPERTIES( TestManifestDiff
        PROPERTIES
        INSTALL_RPATH "${BOOST_LIB_DIRS}"
        BUILD_RPATH "${BOOST_LIB_DIRS}"
        )

target_link_libraries(TestManifestDiff gtest gtest_main gmock )
target_link_libraries(TestManifestDiff manifestdifflib testhelpers -L"${BOOST_LIB_DIRS}" -lboost_filesystem -lboost_system )
gtest_discover_tests(TestManifestDiff)
