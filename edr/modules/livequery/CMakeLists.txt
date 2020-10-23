configure_file(config.h.in config.h)

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
        Logger.h
        Logger.cpp
        EXTRA_PROJECTS queryrunnerimpl
        EXTRA_LIBS ${log4cpluslib} ${protobuflib}
        EXTRA_INCLUDES ${OSQUERYSDK}/include
        )
