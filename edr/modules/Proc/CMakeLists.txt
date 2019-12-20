sophos_add_library(procimpl SHARED
        ProcUtilities.h
        ProcUtilities.cpp
        Logger.h
        Logger.cpp
        EXTRA_INCLUDES ${CMAKE_CURRENT_BINARY_DIR}
        EXTRA_LIBS ${pluginapilib}
)

SET_TARGET_PROPERTIES(procimpl
        PROPERTIES
        INSTALL_RPATH "$ORIGIN:${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "$ORIGIN:${CMAKE_BINARY_DIR}/libs"
        )

add_dependencies(procimpl copy_libs)

