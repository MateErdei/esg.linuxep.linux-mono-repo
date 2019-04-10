include(GoogleTest)
add_executable(ApplicationConfigurationImplTests
        TestApplicationConfiguration.cpp
        )
target_include_directories(ApplicationConfigurationImplTests SYSTEM BEFORE PUBLIC
        "${gtest_SOURCE_DIR}/include"
        "${gmock_SOURCE_DIR}/include"
        ${CMAKE_SOURCE_DIR}
        )
target_include_directories(ApplicationConfigurationImplTests BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${gmock_SOURCE_DIR}")
target_link_libraries(ApplicationConfigurationImplTests gtest gtest_main gmock testhelpers)
gtest_discover_tests(ApplicationConfigurationImplTests)
