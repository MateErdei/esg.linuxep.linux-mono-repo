
sophos_add_library(filewalker SHARED
        FileWalker.cpp
        FileWalker.h
        IFileWalkCallbacks.h
        Logger.cpp
        Logger.h
        EXTRA_LIBS ${STD_FILESYSTEM_IF_REQUIRED} common
        EXTRA_INCLUDES ${CMAKE_SOURCE_DIR}/modules ${CMAKE_CURRENT_BINARY_DIR}
        )
