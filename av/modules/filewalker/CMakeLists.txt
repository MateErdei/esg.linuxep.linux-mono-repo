
sophos_add_library(filewalker SHARED
        FileWalker.cpp
        FileWalker.h
        IFileWalkCallbacks.h
        Logger.cpp
        Logger.h
        EXTRA_LIBS $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:stdc++fs>
        EXTRA_INCLUDES ${CMAKE_SOURCE_DIR}/modules ${CMAKE_CURRENT_BINARY_DIR}
        )
