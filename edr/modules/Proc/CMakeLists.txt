sophos_add_library(procimpl SHARED
        ProcUtilities.h
        ProcUtilities.cpp
        Logger.h
        Logger.cpp
        EXTRA_INCLUDES ${CMAKE_CURRENT_BINARY_DIR}
        EXTRA_LIBS ${pluginapilib}
)

add_dependencies(procimpl copy_libs)

