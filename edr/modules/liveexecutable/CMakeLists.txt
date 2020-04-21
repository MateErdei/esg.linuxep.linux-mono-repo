sophos_add_library(livemain SHARED
        livequery_main.cpp
        livequery_main.h
        EXTRA_LIBS ${log4cpluslib} osqueryclient livequeryimpl
        )
