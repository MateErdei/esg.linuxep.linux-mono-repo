include(GoogleTest)
add_executable(TestUtilityImpl
        TestStringUtils.cpp
        MockFormattedTime.h
        TestUniformIntDistribution.cpp
        TestVectorAsSet.cpp TestTimeUtils.cpp TestOrderedSet.cpp)
target_include_directories(TestUtilityImpl SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(TestUtilityImpl BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${gmock_SOURCE_DIR}")
target_link_libraries(TestUtilityImpl gtest gtest_main gmock utilityimpl)
gtest_discover_tests(TestUtilityImpl)
