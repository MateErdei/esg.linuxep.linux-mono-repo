include(GoogleTest)
add_library(testhelpersobject OBJECT
        MockEventQueue.h
)
SET_TARGET_PROPERTIES(testhelpersobject
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

