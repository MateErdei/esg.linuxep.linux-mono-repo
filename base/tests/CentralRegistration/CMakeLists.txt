include(GoogleTest)

add_executable(centralregistrationtests
        TestCentralRegistration.cpp
        TestCentralRegistrationMain.cpp
        MockAgentAdapter.h
        )
target_include_directories(centralregistrationtests SYSTEM BEFORE PUBLIC ${GTEST_INCLUDE} ${CMAKE_SOURCE_DIR})
target_link_libraries(centralregistrationtests ${GTEST_MAIN_LIBRARY} filesystemimpl testhelpers centralregistrationimpl mcsimpl)
gtest_discover_tests(centralregistrationtests)
SET_TARGET_PROPERTIES(centralregistrationtests
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )