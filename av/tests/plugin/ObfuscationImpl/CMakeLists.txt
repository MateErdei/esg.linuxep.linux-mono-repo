SophosAddTest(TestObfuscation
        ../../common/LogInitializedTests.cpp
        TestBase64.cpp
        MockEvpCipherWrapper.h
        TestCipher.cpp
        TestObscurity.cpp
        PROJECTS obfuscationimpl
        LIBS ${pluginapilib}
        INC_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/modules
        )
