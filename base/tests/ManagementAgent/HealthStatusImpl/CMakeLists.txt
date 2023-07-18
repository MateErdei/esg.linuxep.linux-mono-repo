include(GoogleTest)
add_executable(HealthStatusTests
        HealthTaskTests.cpp
        HealthStatusTests.cpp
        SerialiseFunctionsTests.cpp)

target_include_directories(HealthStatusTests SYSTEM BEFORE PUBLIC ${GTEST_INCLUDE} ${GMOCK_INCLUDE})
target_include_directories(HealthStatusTests BEFORE PUBLIC ${CMAKE_SOURCE_DIR})

target_link_libraries(HealthStatusTests
        ${GTEST_MAIN_LIBRARY}
        xmlutilities
        mcsrouterplugincommunicationimpl
        testhelpers
        )

gtest_discover_tests(HealthStatusTests)
SET_TARGET_PROPERTIES(HealthStatusTests
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )