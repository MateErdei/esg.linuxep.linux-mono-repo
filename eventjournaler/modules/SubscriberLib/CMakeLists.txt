sophos_add_library(subscriberlib SHARED
        Subscriber.cpp
        Subscriber.h
        ISubscriber.h
        Logger.h
        Logger.cpp
        IEventHandler.h
        EventQueuePusher.h
        EventQueuePusher.cpp
        EXTRA_INCLUDES ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/modules heartbeat
        EXTRA_LIBS ${expatlib} ${zmqlib} ${log4cpluslib} ${protobuflib}
        )

SET_TARGET_PROPERTIES(subscriberlib
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )