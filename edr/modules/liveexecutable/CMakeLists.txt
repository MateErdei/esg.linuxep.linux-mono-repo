sophos_add_library(livemain SHARED
        livequery_main.cpp
        livequery_main.h
        Logger.h
        Logger.cpp
        EXTRA_LIBS ${log4cpluslib} osqueryclient livequeryimpl
        )
