SophosAddTest(TestObfuscation
        ../../common/LogInitializedTests.cpp
        TestBase64.cpp
        MockEvpCipherWrapper.h
        TestCipher.cpp
        TestObscurity.cpp
        PROJECTS obfuscationimpl
        LIBS ${pluginapilib}
        )
