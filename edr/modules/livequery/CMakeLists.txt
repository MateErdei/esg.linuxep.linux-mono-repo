sophos_add_library(livequeryimpl SHARED
        IQueryProcessor.h
        QueryProcessor.cpp
        QueryResponse.h
        QueryResponse.cpp
        ResponseDispatcher.h
        ResponseDispatcher.cpp
        ResponseData.h
        ResponseData.cpp
        ResponseMetaData.h
        ResponseMetaData.cpp
        ResponseStatus.h
        ResponseStatus.cpp
        livequery_main.cpp
        livequery_main.h
        Logger.h
        Logger.cpp
        Telemetry.h
        Telemetry.cpp
        EXTRA_LIBS ${log4cpluslib}
        )
