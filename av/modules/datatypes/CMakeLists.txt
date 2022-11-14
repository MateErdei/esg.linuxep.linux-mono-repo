
add_library(datatypes SHARED
        AutoFd.cpp
        AutoFd.h
        Print.h
        sophos_filesystem.h
        Time.cpp
        Time.h
        ISystemCallWrapper.h
        ISystemCallWrapperFactory.h
        SystemCallWrapper.h
        SystemCallWrapperFactory.cpp
        SystemCallWrapperFactory.h
        )

target_link_libraries(datatypes  PUBLIC
        cap
        )

install(TARGETS datatypes
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(datatypes
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "$ORIGIN:$ORIGIN/../../../base/lib64")
