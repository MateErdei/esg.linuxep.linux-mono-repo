sophos_add_library(eventqueuelib SHARED
        EventQueue.h
        EventQueue.cpp
        Logger.cpp
        Logger.h
        EXTRA_INCLUDES ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/modules
        EXTRA_LIBS ${zmqlib} ${log4cpluslib} ${protobuflib}
        )

SET_TARGET_PROPERTIES(eventqueuelib
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )