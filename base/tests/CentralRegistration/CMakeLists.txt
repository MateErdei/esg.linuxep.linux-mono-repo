include(GoogleTest)

add_executable(centralregistrationtestsexe
        TestCentralRegistrationMain.cpp
        )
target_include_directories(centralregistrationtestsexe SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" ${CMAKE_SOURCE_DIR})
target_include_directories(centralregistrationtestsexe BEFORE PUBLIC "${gtest_SOURCE_DIR}")
target_link_libraries(centralregistrationtestsexe gtest gtest_main)
target_link_libraries(centralregistrationtestsexe filesystemimpl testhelpers centralregistrationimpl )
gtest_discover_tests(centralregistrationtestsexe)