include(GoogleTest)
add_executable(TestUtilityImpl
        TestStringUtils.cpp
        MockFormattedTime.h
        TestUniformIntDistribution.cpp
        TestVectorAsSet.cpp
        TestWaitForUtils.cpp
        TestTimeUtils.cpp
        TestOrderedSet.cpp)
target_include_directories(TestUtilityImpl SYSTEM BEFORE PUBLIC ${GTEST_INCLUDE} ${GMOCK_INCLUDE})
target_link_libraries(TestUtilityImpl ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY} ${GMOCK_LIBRARY} utilityimpl filesystemimpl testhelpers)
gtest_discover_tests(TestUtilityImpl)
