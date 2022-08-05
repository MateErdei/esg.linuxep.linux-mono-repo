include(GoogleTest)

SophosAddTest(TestOnAccessImpl
        ../../common/LogInitializedTests.cpp
        TestClientSocketWrapper.cpp
        PROJECTS onaccessimpl
        INC_DIRS
        ${CMAKE_SOURCE_DIR}/modules
        ${CMAKE_SOURCE_DIR}/tests
        LIBS pthread
        )

SET_TARGET_PROPERTIES(TestOnAccessImpl
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

