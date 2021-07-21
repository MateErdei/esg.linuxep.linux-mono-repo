sophos_add_library(eventwriterlib SHARED
        Logger.h
        Logger.cpp
        IEventWriterWorker.h
        EventWriterWorker.h
        EventWriterWorker.cpp
        IEventQueuePopper.h
        EventQueuePopper.h
        EventQueuePopper.cpp
        EXTRA_PROJECTS eventjournal
        EXTRA_INCLUDES ${CMAKE_CURRENT_BINARY_DIR}
        EXTRA_LIBS ${zmqlib} ${log4cpluslib} ${protobuflib}
        )

SET_TARGET_PROPERTIES(eventwriterlib
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )