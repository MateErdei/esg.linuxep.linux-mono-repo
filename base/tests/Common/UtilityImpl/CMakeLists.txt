include(GoogleTest)
add_executable(TestUtilityImpl
        MockFormattedTime.h
        TestOrderedSet.cpp
        TestStringUtils.cpp
        TestTimeUtils.cpp
        TestUniformIntDistribution.cpp
        TestUuid.cpp
        TestVectorAsSet.cpp
        TestWaitForUtils.cpp
        )
target_include_directories(TestUtilityImpl SYSTEM BEFORE PUBLIC ${GTEST_INCLUDE} ${GMOCK_INCLUDE})
target_link_libraries(TestUtilityImpl ${GTEST_MAIN_LIBRARY} utilityimpl filesystemimpl testhelpers)
gtest_discover_tests(TestUtilityImpl)
SET_TARGET_PROPERTIES(TestUtilityImpl
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
