include(GoogleTest)

SophosAddTest(TestDatatypes
        MockSysCalls.h
        TestAutoFd.cpp
        TestTime.cpp
        PROJECTS datatypes
        LIBS ${datatypes} rt pthread
        INC_DIRS
        ${CMAKE_SOURCE_DIR}/modules
        )

add_dependencies(TestDatatypes copy_libs)

SET_TARGET_PROPERTIES(TestDatatypes
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )