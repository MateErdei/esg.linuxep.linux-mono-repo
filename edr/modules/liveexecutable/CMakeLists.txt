sophos_add_library(livemain SHARED
        livequery_main.cpp
        livequery_main.h
        Logger.cpp
        Logger.h
        EXTRA_LIBS ${log4cpluslib} ${zmqlib} osqueryclient livequeryimpl ${JSONCPP_LIBRARY}
        EXTRA_INCLUDES ${CMAKE_BINARY_DIR}/modules
        )
