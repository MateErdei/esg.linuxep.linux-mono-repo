add_library(mountinfoimpl SHARED
        DeviceUtil.cpp
        DeviceUtil.h
        Drive.cpp
        Drive.h
        Logger.cpp
        Logger.h
        Mounts.cpp
        Mounts.h
        MountsList.cpp
        MountsList.h
        SystemPaths.h
        SystemPathsFactory.cpp
        SystemPathsFactory.h
        ../mountinfo/IMountInfo.h
        ../mountinfo/IMountPoint.h
        ../mountinfo/ISystemPaths.h
        )

target_include_directories(mountinfoimpl PUBLIC
        ${CMAKE_SOURCE_DIR}/modules
        ${CMAKE_BINARY_DIR}/modules
        ${pluginapiinclude}
        )

target_link_libraries(mountinfoimpl
        common)

install(TARGETS mountinfoimpl
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(mountinfoimpl
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "$ORIGIN")
