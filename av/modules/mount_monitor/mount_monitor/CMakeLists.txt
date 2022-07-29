add_library(mountmonitor SHARED
        Logger.cpp
        Logger.h
        MountMonitor.cpp
        MountMonitor.h
        )

target_include_directories(mountmonitor PUBLIC
        ${CMAKE_SOURCE_DIR}/modules
        ${CMAKE_BINARY_DIR}/modules
        ${pluginapiinclude}
        )

target_link_libraries(mountmonitor
        mountinfoimpl
        common)

install(TARGETS mountmonitor
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(mountmonitor
        PROPERTIES
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../../../base/lib64"
        BUILD_RPATH "$ORIGIN")
