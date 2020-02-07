
add_library(datatypes SHARED
        AutoFd.cpp
        AutoFd.h
        Print.h
        sophos_filesystem.h
        ../manager/scheduler/IScanComplete.h)

install(TARGETS datatypes
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(datatypes
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "$ORIGIN:$ORIGIN/../../../base/lib64")
