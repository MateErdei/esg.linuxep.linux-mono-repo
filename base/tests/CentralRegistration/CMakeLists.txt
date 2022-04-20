include(GoogleTest)

add_executable(centralregistrationtests
        TestCentralRegistration.cpp
        TestCentralRegistrationMain.cpp
        )
target_include_directories(centralregistrationtests SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" ${CMAKE_SOURCE_DIR})
target_include_directories(centralregistrationtests BEFORE PUBLIC "${gtest_SOURCE_DIR}")
target_link_libraries(centralregistrationtests gtest gtest_main gmock filesystemimpl testhelpers centralregistrationimpl mcsimpl)
gtest_discover_tests(centralregistrationtests)
SET_TARGET_PROPERTIES(centralregistrationtests
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )