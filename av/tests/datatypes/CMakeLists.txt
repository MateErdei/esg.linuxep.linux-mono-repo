include(GoogleTest)

SophosAddTest(TestDatatypes
        TestTime.cpp
        TestAutoFD.cpp
        MockSysCalls.h
        PROJECTS datatypes
        LIBS ${datatypes} rt
        INC_DIRS
        ${CMAKE_SOURCE_DIR}/modules
        )

add_dependencies(TestDatatypes copy_libs)

SET_TARGET_PROPERTIES(TestDatatypes
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )