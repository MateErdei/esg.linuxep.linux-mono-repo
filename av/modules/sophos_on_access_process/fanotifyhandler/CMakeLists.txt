
add_library(fanotifyhandler SHARED
        EventReaderThread.cpp
        EventReaderThread.h
        ExclusionCache.cpp
        ExclusionCache.h
        ExecutablePathCache.cpp
        ExecutablePathCache.h
        FanotifyHandler.cpp
        FanotifyHandler.h
        IFanotifyHandler.h
        Logger.cpp
        Logger.h
        ProcUtils.cpp
        ProcUtils.h
        )

target_include_directories(fanotifyhandler PUBLIC
        ${CMAKE_SOURCE_DIR}/modules
        ${pluginapiinclude}
        )

target_link_libraries(fanotifyhandler
        common onaccessimpl)

install(TARGETS fanotifyhandler
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(fanotifyhandler
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN"
        )