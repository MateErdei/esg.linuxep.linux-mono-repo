sophos_add_library(writerlib SHARED
        IEventQueuePopper.h
        EventQueuePopper.h
        EventQueuePopper.cpp
        EXTRA_INCLUDES ${CMAKE_CURRENT_BINARY_DIR}
        EXTRA_LIBS ${zmqlib} ${log4cpluslib} ${protobuflib}
        )

SET_TARGET_PROPERTIES(writerlib
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )