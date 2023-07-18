include(GoogleTest)
add_executable(TestObfuscation
        TestBase64.cpp
        MockEvpCipherWrapper.h
        TestCipher.cpp
        TestObscurity.cpp)
target_include_directories(TestObfuscation SYSTEM BEFORE PUBLIC ${GMOCK_INCLUDE} ${GTEST_INCLUDE} ${CMAKE_SOURCE_DIR})
target_link_libraries(TestObfuscation ${GTEST_MAIN_LIBRARY} obfuscationimpl logging)
gtest_discover_tests(TestObfuscation)
SET_TARGET_PROPERTIES(TestObfuscation
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )