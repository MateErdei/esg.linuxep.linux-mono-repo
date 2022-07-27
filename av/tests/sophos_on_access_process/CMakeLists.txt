include(GoogleTest)

SophosAddTest(TestSophosOnAccessProcessImpl
        ../common/LogInitializedTests.cpp
        TestSoapdBootstrap.cpp
        PROJECTS soapdbootstrap
        INC_DIRS
        ${CMAKE_SOURCE_DIR}/modules
        ${CMAKE_SOURCE_DIR}/tests
        LIBS pthread
        )

SET_TARGET_PROPERTIES(TestSophosOnAccessProcessImpl
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )