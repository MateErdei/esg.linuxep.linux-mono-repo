include(GoogleTest)
add_executable(TestDiagnose
        TestCheckForTar.cpp
        TestGatherFiles.cpp
        SystemCommandsTests.cpp)
target_include_directories(TestDiagnose BEFORE PUBLIC "${GTEST_INCLUDE}" "${GMOCK_INCLUDE}" ${CMAKE_SOURCE_DIR})
target_link_libraries(TestDiagnose
        ${GTEST_MAIN_LIBRARY}
        diagnoseimpl
        testhelpers
        )
gtest_discover_tests(TestDiagnose)
SET_TARGET_PROPERTIES(TestDiagnose
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )