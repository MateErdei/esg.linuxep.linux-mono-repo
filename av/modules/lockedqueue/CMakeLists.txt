
add_library(lockedqueue SHARED
        LockedQueue.cpp
        LockedQueue.h
        )

target_include_directories(lockedqueue PUBLIC ${pluginapiinclude})
target_link_libraries(lockedqueue ${pluginapilib})

install(TARGETS lockedqueue
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(lockedqueue
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "$ORIGIN:$ORIGIN/../../../base/lib64")
