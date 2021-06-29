sophos_add_library(subscriberlib SHARED
        Subscriber.cpp
        Subscriber.h
        Logger.h
        Logger.cpp
        EXTRA_INCLUDES ${CMAKE_CURRENT_BINARY_DIR}
        EXTRA_LIBS ${zmqlib} ${log4cpluslib} ${protobuf}
        )

SET_TARGET_PROPERTIES(subscriberlib
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )