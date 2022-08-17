
add_library(fanotifyhandler SHARED
        EventReaderThread.cpp
        EventReaderThread.h
        FANotifyHandler.cpp
        FANotifyHandler.h
        Logger.cpp
        Logger.h)

target_link_libraries(fanotifyhandler  PUBLIC
        mountmonitor
        )

install(TARGETS fanotifyhandler
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(fanotifyhandler
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../../../base/lib64:/opt/sophos-spl/plugins/av/lib64:/opt/sophos-spl/base/lib64"
        )