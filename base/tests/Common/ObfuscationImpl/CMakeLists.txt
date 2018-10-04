include(GoogleTest)
add_executable(TestObfuscation
        TestBase64.cpp
        MockEvpCipherWrapper.h
        TestCipher.cpp
        TestObscurity.cpp)
target_include_directories(TestObfuscation SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include" ${CMAKE_SOURCE_DIR})
target_include_directories(TestObfuscation BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${gmock_SOURCE_DIR}" )
target_link_libraries(TestObfuscation gtest gtest_main gmock obfuscationimpl logging)
gtest_discover_tests(TestObfuscation)