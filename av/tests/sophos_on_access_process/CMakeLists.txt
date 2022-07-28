include(GoogleTest)

SophosAddTest(TestSophosOnAccessProcessImpl
        ../common/LogInitializedTests.cpp
        TestSoapdBootstrap.cpp
        TestOnAccessConfigMonitor.cpp
        PROJECTS soapdbootstrap
        INC_DIRS
        ${CMAKE_SOURCE_DIR}/modules
        ${CMAKE_SOURCE_DIR}/tests
        ${pluginapiinclude}
        ${testhelpersinclude}
        LIBS pthread ${pluginapilib} ${testhelperslib}
        )

SET_TARGET_PROPERTIES(TestSophosOnAccessProcessImpl
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )