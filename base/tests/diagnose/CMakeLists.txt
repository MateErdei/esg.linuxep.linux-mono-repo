include(GoogleTest)
add_executable(TestDiagnose
        TestCheckForTar.cpp
        )
target_include_directories(TestDiagnose SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(TestDiagnose BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${gmock_SOURCE_DIR}" ${CMAKE_SOURCE_DIR})
target_link_libraries(TestDiagnose gtest gmock gtest_main)
target_link_libraries(TestDiagnose diagnoseimpl)
gtest_discover_tests(TestDiagnose)
