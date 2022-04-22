include(GoogleTest)
add_executable(ApplicationConfigurationImplTests
        TestApplicationConfiguration.cpp
        )
target_include_directories(ApplicationConfigurationImplTests SYSTEM BEFORE PUBLIC
        ${GMOCK_INCLUDE}
        ${GTEST_INCLUDE}
        ${CMAKE_SOURCE_DIR}
        )
target_link_libraries(ApplicationConfigurationImplTests ${GMOCK_LIBRARY} ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY} testhelpers)
gtest_discover_tests(ApplicationConfigurationImplTests)
SET_TARGET_PROPERTIES(ApplicationConfigurationImplTests
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )