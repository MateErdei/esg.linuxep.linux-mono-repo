include(GoogleTest)
add_executable(TestObfuscation
        ../../common/LogInitializedTests.cpp
        TestBase64.cpp
        MockEvpCipherWrapper.h
        TestCipher.cpp
        TestObscurity.cpp)
target_include_directories(TestObfuscation SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include" )
target_include_directories(TestObfuscation BEFORE PUBLIC
        "${gtest_SOURCE_DIR}"
        "${gmock_SOURCE_DIR}"
        ${CMAKE_SOURCE_DIR}/modules
        ${CMAKE_SOURCE_DIR}
        )
target_link_libraries(TestObfuscation gtest gtest_main gmock obfuscationimpl ${pluginapilib})
gtest_discover_tests(TestObfuscation)