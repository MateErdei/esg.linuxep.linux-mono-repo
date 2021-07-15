sophos_add_library(subscriberlib SHARED
        Subscriber.cpp
        Subscriber.h
        ISubscriber.h
        Logger.h
        Logger.cpp
        IEventQueuePusher.h
        EventQueuePusher.h
        EventQueuePusher.cpp
        ../WriterLib/IEventQueuePopper.h
        ../WriterLib/EventQueuePopper.h
        ../WriterLib/EventQueuePopper.cpp
        EXTRA_INCLUDES ${CMAKE_CURRENT_BINARY_DIR}
        EXTRA_LIBS ${zmqlib} ${log4cpluslib} ${protobuflib}
        )

SET_TARGET_PROPERTIES(subscriberlib
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )