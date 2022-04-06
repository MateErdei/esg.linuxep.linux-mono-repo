include(GoogleTest)

add_executable(centralregistrationtests
        TestCentralRegistration.cpp
        TestCentralRegistrationMain.cpp
        )
target_include_directories(centralregistrationtests SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" ${CMAKE_SOURCE_DIR})
target_include_directories(centralregistrationtests BEFORE PUBLIC "${gtest_SOURCE_DIR}")
target_link_libraries(centralregistrationtests gtest gtest_main gmock)
target_link_libraries(centralregistrationtests filesystemimpl testhelpers centralregistrationimpl )
gtest_discover_tests(centralregistrationtests)