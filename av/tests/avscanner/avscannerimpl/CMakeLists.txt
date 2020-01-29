include(GoogleTest)

SophosAddTest(TestOptions
        TestOptions.cpp
        PROJECTS avscannerimpl
        INC_DIRS
        ${CMAKE_SOURCE_DIR}/modules
        )

SET_TARGET_PROPERTIES(TestOptions
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
