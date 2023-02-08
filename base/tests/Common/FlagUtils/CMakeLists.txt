include(GoogleTest)

add_executable(FlagUtilsTests
        TestFlagUtils.cpp)

target_include_directories(FlagUtilsTests SYSTEM BEFORE PUBLIC ${GTEST_INCLUDE} ${GMOCK_INCLUDE} ${LOG4CPLUS_INCLUDE_DIR} "${PROJECT_SOURCE_DIR}")
target_link_libraries(FlagUtilsTests ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY} testhelpers flagutilsobject)

SET_TARGET_PROPERTIES(FlagUtilsTests
        PROPERTIES
        INSTALL_RPATH "${SUL_LIBRARY_DIRS}"
        BUILD_RPATH "${SUL_LIBRARY_DIRS}"
        )

gtest_discover_tests(FlagUtilsTests)