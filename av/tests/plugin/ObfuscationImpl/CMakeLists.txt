include(GoogleTest)
add_executable(TestObfuscation
        ../../common/LogInitializedTests.cpp
        TestBase64.cpp
        MockEvpCipherWrapper.h
        TestCipher.cpp
        TestObscurity.cpp)
target_include_directories(TestObfuscation SYSTEM BEFORE PUBLIC ${GMOCK_INCLUDE} ${GTEST_INCLUDE})
target_include_directories(TestObfuscation BEFORE PUBLIC
        ${CMAKE_SOURCE_DIR}/modules
        ${CMAKE_SOURCE_DIR}
        )
target_link_libraries(TestObfuscation ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY} ${GMOCK_LIBRARY} obfuscationimpl ${pluginapilib} pthread)
gtest_discover_tests(TestObfuscation)