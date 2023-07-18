include(GoogleTest)

add_executable(FlagUtilsTests
        TestFlagUtils.cpp)

target_include_directories(FlagUtilsTests SYSTEM BEFORE PUBLIC ${GTEST_INCLUDE} ${GMOCK_INCLUDE} ${LOG4CPLUS_INCLUDE_DIR} "${PROJECT_SOURCE_DIR}")
target_link_libraries(FlagUtilsTests ${GTEST_MAIN_LIBRARY} testhelpers flagutilsobject)

SET_TARGET_PROPERTIES(FlagUtilsTests
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

gtest_discover_tests(FlagUtilsTests)